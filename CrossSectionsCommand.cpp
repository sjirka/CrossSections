#include "CrossSectionsCommand.h"
#include "../_library/SData.h"

#include <maya\MSyntax.h>
#include <maya\MSelectionList.h>
#include <maya\MItSelectionList.h>
#include <maya\MFnDagNode.h>
#include <maya\MGlobal.h>
#include <maya\MPlugArray.h>
#include <maya\MFnTransform.h>
#include <maya\MTransformationMatrix.h>
#include <maya\MArgList.h>
#include <maya\MDataHandle.h>
#include <maya\MItDependencyGraph.h>
#include <maya\MItDag.h>
#include <maya\MDGMessage.h>

CrossSectionsCommand::CrossSectionsCommand(){
}

CrossSectionsCommand::~CrossSectionsCommand(){
}

void* CrossSectionsCommand::creator() {
	return new CrossSectionsCommand();
}

MSyntax CrossSectionsCommand::newSyntax() {
	MSyntax syntax;

	// Create / edit
	syntax.addFlag(COMMAND_NUM_PLANES_FLAG, COMMAND_NUM_PLANES_FLAG_LONG, MSyntax::kLong);
	syntax.addFlag(COMMAND_SPACING_FLAG, COMMAND_SPACING_FLAG_LONG, MSyntax::kDouble);
	syntax.addFlag(COMMAND_TRANSLATE_FLAG, COMMAND_TRANSLATE_FLAG_LONG, MSyntax::kDistance, MSyntax::kDistance, MSyntax::kDistance);
	syntax.addFlag(COMMAND_ROTATE_FLAG, COMMAND_ROTATE_FLAG_LONG, MSyntax::kAngle, MSyntax::kAngle, MSyntax::kAngle);
	syntax.addFlag(COMMAND_LINEWIDTH_FLAG, COMMAND_LINEWIDTH_FLAG_LONG, MSyntax::kDouble);
	syntax.addFlag(COMMAND_COLOR_FLAG, COMMAND_COLOR_FLAG_LONG, MSyntax::kDouble, MSyntax::kDouble, MSyntax::kDouble);
	syntax.addFlag(COMMAND_COMB_FLAG, COMMAND_COMB_FLAG_LONG, MSyntax::kBoolean);
	syntax.addFlag(COMMAND_SAMPLES_FLAG, COMMAND_SAMPLES_FLAG_LONG, MSyntax::kLong);
	syntax.addFlag(COMMAND_SCALE_FLAG, COMMAND_SCALE_FLAG_LONG, MSyntax::kDouble);
	syntax.addFlag(COMMAND_SMOOTH_FLAG, COMMAND_SMOOTH_FLAG_LONG, MSyntax::kBoolean);
	syntax.addFlag(COMMAND_RADIUS_FLAG, COMMAND_RADIUS_FLAG_LONG, MSyntax::kLong);
	syntax.addFlag(COMMAND_CLIP_FLAG, COMMAND_CLIP_FLAG_LONG, MSyntax::kBoolean);
	// Edit
	syntax.addFlag(COMMAND_ADD_FLAG, COMMAND_ADD_FLAG_LONG, MSyntax::kString);
	syntax.makeFlagMultiUse(COMMAND_ADD_FLAG);
	syntax.addFlag(COMMAND_REMOVE_FLAG, COMMAND_REMOVE_FLAG_LONG, MSyntax::kLong);
	syntax.makeFlagMultiUse(COMMAND_REMOVE_FLAG);
	// Query
	syntax.addFlag(COMMAND_INPUTS_FLAG, COMMAND_INPUTS_FLAG_LONG, MSyntax::kNoArg);
	syntax.addFlag(COMMAND_OBJECTS_FLAG, COMMAND_OBJECTS_FLAG_LONG, MSyntax::kNoArg);
	syntax.addFlag(COMMAND_CURVES_FLAG, COMMAND_CURVES_FLAG_LONG, MSyntax::kLong);
	syntax.makeFlagMultiUse(COMMAND_CURVES_FLAG);
	syntax.makeFlagQueryWithFullArgs(COMMAND_CURVES_FLAG, false);

	syntax.addArg(MSyntax::kString);

	syntax.useSelectionAsDefault(true);
	syntax.setObjectType(MSyntax::kSelectionList, 1);
	syntax.enableEdit(true);
	syntax.enableQuery(true);

	return syntax;
}

MStatus CrossSectionsCommand::doIt(const MArgList& args) {
	MStatus status;

	MArgDatabase argData(syntax(), args, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	m_isEdit = argData.isEdit(&status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	m_isQuery = argData.isQuery(&status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	m_isCreation = (m_isEdit || m_isQuery) ? false : true;

	if (m_isEdit && m_isQuery) {
		setResult(false);
		displayError("Invalid syntax");
		return MS::kFailure;
	}

	//  Get target plugin node
	MString name = argData.commandArgumentString(0, &status);

	// On creation connect all objects in selection
	if (m_isCreation) {
		MSelectionList selection;
		status = argData.getObjects(selection);
		if (MS::kSuccess != status || 0==selection.length()) {
			displayError("Nothing selected");
			return MS::kFailure;
		}

		
		MDagPathArray filteredNodes;
		status = filterSelection(selection, filteredNodes);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		if (filteredNodes.length() == 0) {
			setResult(false);
			displayError("No supported geometry selected");
			return MS::kSuccess;
		}

		status = createPluginNode(name);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		status = connectGeometry(filteredNodes);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		// Place to center of geometry
		if (!argData.isFlagSet(COMMAND_TRANSLATE_FLAG))
			m_useBBox = true;
	}

	if (m_isEdit || m_isQuery) {
		status = getPluginNode(name);
		if (MS::kSuccess != status)
			return MS::kSuccess;
	}

	if (m_isCreation || m_isEdit) {
		status = setAttributes(argData);
		CHECK_MSTATUS_AND_RETURN_IT(status);
	}
	else if (m_isQuery) {
		status = queryAttributes(argData);
		CHECK_MSTATUS_AND_RETURN_IT(status);
	}

	return redoIt();

	return MS::kSuccess;
}

MStatus CrossSectionsCommand::redoIt() {
	MStatus status;

	status = m_dagMod.doIt();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	if (m_isCreation || m_hasOutput) {
		MFnDagNode fnNode((m_hasOutput)?m_oOutput:m_oTransform, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		if (m_useBBox) {
			MPlug pTranslate = fnNode.findPlug("translate", &status);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			double center[4];
			m_bBox.center().get(center);

			for (unsigned int i = 0; i < pTranslate.numChildren(); i++)
				status = pTranslate.child(i).setDouble(center[i]);
		}

		MGlobal::executeCommand("select "+ fnNode.name(), false, false);
		setResult(fnNode.name());
	}
	if (m_isEdit)
		setResult(true);

	return MS::kSuccess;
}

MStatus CrossSectionsCommand::undoIt() {
	MStatus status;

	status = m_dagMod.undoIt();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	return MS::kSuccess;
}

bool CrossSectionsCommand::isUndoable() const {
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Create / get plugin node ///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

MStatus CrossSectionsCommand::createNode(const char *type, const char *name, MObject& parent, MObject& node) {
	MStatus status;

	node = m_dagMod.createNode(type, parent, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	MFnDagNode fnNode(node, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	fnNode.setName(name);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	return MS::kSuccess;
}

MStatus CrossSectionsCommand::createPluginNode(MString& name) {
	MStatus status;

	name = (name != "") ? name : NODE_NAME;
	MString nodeName = name + "Shape";

	status = createNode("transform", name.asChar(), MObject::kNullObj, m_oTransform);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	m_oNode = m_dagMod.createNode(CrossSectionsNode::id, m_oTransform, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	MFnDagNode fnCrossSections(m_oNode);
	fnCrossSections.setName(nodeName);

	return MS::kSuccess;
}

MStatus CrossSectionsCommand::getPluginNode(MString& name) {
	MStatus status;

	MDagPath path;
	MSelectionList selection;
	status = selection.add(name);
	if (MS::kSuccess != status) {
		displayError("object doesn't exist");
		return MS::kInvalidParameter;
	}

	status = selection.getDagPath(0, path);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = path.extendToShape();
	CHECK_MSTATUS_AND_RETURN_IT(status);
	m_oTransform = path.transform(&status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	MFnDagNode fnNode(path, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	if (CrossSectionsNode::id != fnNode.typeId()) {
		displayError("invalid object type");
		return MS::kInvalidParameter;
	}

	m_oNode = path.node(&status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	return MS::kSuccess;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Make connections ///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

MStatus CrossSectionsCommand::getShapes(MDagPath& path, MDagPathArray& shapes){
	MStatus status;

	shapes.clear();

	MItDag itDag;
	for (itDag.reset(path); !itDag.isDone(); itDag.next()) {
		MObject node = itDag.currentItem();

		if (node.apiType() != MFn::kMesh &&
			node.apiType() != MFn::kNurbsSurface &&
			node.apiType() != MFn::kSubdiv)
			continue;

		MDagPath path;
		itDag.getPath(path);

		if(path.isVisible())
			shapes.append(path);
	}

	return MS::kSuccess;
}

MStatus CrossSectionsCommand::filterSelection(MSelectionList& selection, MDagPathArray& filteredNodes) {
	MStatus status;

	for (unsigned int i = 0; i < selection.length(); i++) {
		MDagPath path;
		status = selection.getDagPath(i, path);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		MDagPathArray shapes;
		status = getShapes(path, shapes);

		for (unsigned int o = 0; o < shapes.length(); o++)
			filteredNodes.append(shapes[o]);
	}

	return MS::kSuccess;
}


bool CrossSectionsCommand::nodeIsConnectedToInput(MDagPath& path) {
	MStatus status;

	MPlug matrixPlug = findWorldMatrixPlug(path, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	MItDependencyGraph itGraph(matrixPlug, MFn::kPluginLocatorNode);

	for (itGraph.reset(); !itGraph.isDone(); itGraph.next())
		if (m_oNode == itGraph.thisNode())
			return true;

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Connect / disconnect ///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// Connect
MStatus CrossSectionsCommand::connectGeometry(MDagPathArray& nodes) {
	MStatus status;

	MFnDagNode fnNode(m_oNode, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	MPlug pInput = fnNode.findPlug(CrossSectionsNode::aInput, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	unsigned int index = 0;
	for (unsigned int i = 0; i < nodes.length(); i++) {
		
		if (nodeIsConnectedToInput(nodes[i])) {
			displayInfo(nodes[i].partialPathName() + " is already connected.");
			continue;
		}

		status = findFreeInputPlug(pInput, index);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		MPlug pInputElement = pInput.elementByLogicalIndex(index, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		status = connectGeometry(nodes[i], pInputElement);

		if( MS::kSuccess == status)
			index++;
	}

	return MS::kSuccess;
}

MStatus CrossSectionsCommand::connectGeometry(MDagPath& path, MPlug& pInputElementPlug) {
	MStatus status;

	MDGModifier dgMod;

	MFnDagNode fnGeom(path);
	MPlug pOutGeometry;

	MObject helperNode;

	// Connect each geometry type through a conversion node to generate smooth mesh
	switch (path.apiType())
	{
	case MFn::kMesh: {
		// Pick between outMesh and outSmoothMesh based on displaySmoothMesh attribute
		helperNode = dgMod.createNode("choice", &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		dgMod.doIt();

		MFnDependencyNode fnChoice(helperNode);

		MPlug pOut = fnGeom.findPlug("outMesh", &status);
		MPlug pOutSmooth = fnGeom.findPlug("outSmoothMesh", &status);
		MPlug pDispSmooth = fnGeom.findPlug("displaySmoothMesh", &status);

		MPlug pSelector = fnChoice.findPlug("selector");
		MPlug pInput = fnChoice.findPlug("input");
		MPlug pInput0 = pInput.elementByLogicalIndex(0);
		MPlug pInput1 = pInput.elementByLogicalIndex(1);
		MPlug pInput2 = pInput.elementByLogicalIndex(2);

		m_dagMod.connect(pOut, pInput0);
		m_dagMod.connect(pOutSmooth, pInput1);
		m_dagMod.connect(pOutSmooth, pInput2);
		m_dagMod.connect(pDispSmooth, pSelector);

		pOutGeometry = fnChoice.findPlug("output", &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		break;}
	case MFn::kNurbsSurface: {
		// Perform nurbs tesselation based on display options
		helperNode = dgMod.createNode("nurbsTessellate", &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		dgMod.doIt();
	
		MFnDependencyNode fnTesselate(helperNode);
		MPlug pFormat = fnTesselate.findPlug("format",&status);
		MPlug pUNumber = fnTesselate.findPlug("uNumber", &status);
		MPlug pVNumber = fnTesselate.findPlug("vNumber", &status);
		MPlug pInSurface = fnTesselate.findPlug("inputSurface", &status);
		MPlug pCrvShaded = fnGeom.findPlug("curvePrecisionShaded", &status);
		MPlug pLocal = fnGeom.findPlug("local", &status);
	
		status = m_dagMod.newPlugValueInt(pFormat, 2);
		status = m_dagMod.connect(pCrvShaded, pUNumber);
		status = m_dagMod.connect(pCrvShaded, pVNumber);
		status = m_dagMod.connect(pLocal, pInSurface);

		pOutGeometry = fnTesselate.findPlug("outputPolygon", &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		break;}
	case MFn::kSubdiv: {
		// Perform subd to poly conversion based on display options
		helperNode = dgMod.createNode("subdivToPoly", &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		dgMod.doIt();

		MFnDependencyNode fnSub2Poly(helperNode);
		MPlug pSampleCount = fnSub2Poly.findPlug("sampleCount", &status);
		MPlug pInSubdiv = fnSub2Poly.findPlug("inSubdiv", &status);
		MPlug pDispResolution = fnGeom.findPlug("dispResolution", &status);
		MPlug pOutSubdiv = fnGeom.findPlug("outSubdiv", &status);

		status = m_dagMod.connect(pDispResolution, pSampleCount);
		status = m_dagMod.connect(pOutSubdiv, pInSubdiv);

		pOutGeometry = fnSub2Poly.findPlug("outMesh", &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		break;}
	}

	MPlug pInputGeometry = pInputElementPlug.child(CrossSectionsNode::aInputMesh, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = m_dagMod.connect(pOutGeometry, pInputGeometry);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Connect matching plug
	MPlug pInputMatrix = pInputElementPlug.child(CrossSectionsNode::aInputMatrix, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	MPlug pWorldMatrix = findWorldMatrixPlug(path, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = m_dagMod.connect(pWorldMatrix, pInputMatrix);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	if (m_isCreation) {
		MFnDagNode fnTransform(path.transform());
		m_bBox.expand(fnTransform.boundingBox());
	}

	return MS::kSuccess;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Plug helpers ///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

MPlug CrossSectionsCommand::findWorldMatrixPlug(MDagPath& path, MStatus *status) {
	MFnDagNode fnGoemetry(path, status);
	CHECK_MSTATUS_AND_RETURN(*status, MPlug());

	MPlug pWorldMatrix = fnGoemetry.findPlug("worldMatrix", status);
	CHECK_MSTATUS_AND_RETURN(*status, MPlug());
	unsigned int plugIndex = 0;
	for (unsigned int i = 0; i < fnGoemetry.instanceCount(true); i++) {
		if (fnGoemetry.parent(i) == path.transform()) {
			plugIndex = i;
			break;
		}
	}
	return pWorldMatrix.elementByLogicalIndex(plugIndex, status);
}

MStatus CrossSectionsCommand::findFreeInputPlug(MPlug& inputPlug, unsigned int &index) {
	MStatus status;
	
	for (unsigned int p = index; ; p++) {
		MPlug pInputElement = inputPlug.elementByLogicalIndex(p, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		MPlug pInputGeomery = pInputElement.child(CrossSectionsNode::aInputMesh, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		if (!pInputGeomery.isConnected()) {
			index = p;
			return MS::kSuccess;
		}
	}

	return MS::kFailure;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Edit / query ///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

MStatus CrossSectionsCommand::setAttributes(MArgDatabase& argData) {
	MStatus status;

	MFnDagNode fnNode(m_oNode, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	MFnDagNode fnTransform(m_oTransform, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Set numeric flags/attributes
	for (auto &flag : m_attrFlags) {
		if (argData.isFlagSet(flag.first)) {
			MPlug attrPlug;
			if (COMMAND_ROTATE_FLAG == flag.first)
				attrPlug = fnTransform.findPlug("rotate", &status);
			else if (COMMAND_TRANSLATE_FLAG == flag.first)
				attrPlug = fnTransform.findPlug("translate", &status);
			else
				attrPlug = fnNode.findPlug(flag.second, &status);
			CHECK_MSTATUS_AND_RETURN_IT(status);

			status = setFlagAttr(argData, flag.first, attrPlug);
			CHECK_MSTATUS_AND_RETURN_IT(status);
		}
	}

	// Add geometry
	if (argData.isFlagSet(COMMAND_ADD_FLAG)) {
		MSelectionList geometry;
		for (unsigned int i = 0; i < argData.numberOfFlagUses(COMMAND_ADD_FLAG); i++) {
			MArgList argList;
			argData.getFlagArgumentList(COMMAND_ADD_FLAG, i, argList);
			MString name = argList.asString(0);
			status = geometry.add(name);
			if (MS::kSuccess != status)
				displayError(name + " doesn't exist");
		}
		MDagPathArray filteredNodes;
		status = filterSelection(geometry, filteredNodes);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		status = connectGeometry(filteredNodes);
		CHECK_MSTATUS_AND_RETURN_IT(status);
	}
	// Remove geometry
	if (argData.isFlagSet(COMMAND_REMOVE_FLAG)) {
		MPlug pInput = fnNode.findPlug(CrossSectionsNode::aInput, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		for (unsigned int i = 0; i < argData.numberOfFlagUses(COMMAND_REMOVE_FLAG); i++) {
			MArgList argList;
			argData.getFlagArgumentList(COMMAND_REMOVE_FLAG, i, argList);
			int id = argList.asInt(0);
			MPlug pInputElement = pInput.elementByLogicalIndex(id, &status);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			status = m_dagMod.removeMultiInstance(pInputElement, true);
			CHECK_MSTATUS_AND_RETURN_IT(status);
		}
	}

	return MS::kSuccess;
}

MStatus CrossSectionsCommand::queryAttributes(MArgDatabase& argData) {
	MStatus status;

	MFnDagNode fnNode(m_oNode, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	MFnDagNode fnTransform(m_oTransform, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Query numeric attributes
	bool hasResult = false;
	for (auto &flag : m_attrFlags) {
		if (argData.isFlagSet(flag.first) && !hasResult) {
			MPlug attrPlug;
			if (COMMAND_ROTATE_FLAG == flag.first)
				attrPlug = fnTransform.findPlug("rotate", &status);
			else if (COMMAND_TRANSLATE_FLAG == flag.first)
				attrPlug = fnTransform.findPlug("translate", &status);
			else
				attrPlug = fnNode.findPlug(flag.second, &status);
			CHECK_MSTATUS_AND_RETURN_IT(status);

			status = queryAttrValue(attrPlug);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			hasResult = true;
		}
	}
	
	// Query inputs
	if(!hasResult && (argData.isFlagSet(COMMAND_INPUTS_FLAG) || argData.isFlagSet(COMMAND_OBJECTS_FLAG))){
		MPlug pInput = fnNode.findPlug(CrossSectionsNode::aInput, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		for (unsigned int i = 0; i < pInput.numElements(); i++) {
			MPlug pInputElement = pInput.elementByPhysicalIndex(i, &status);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			MPlug pInputGeometry = pInputElement.child(CrossSectionsNode::aInputMesh, &status);
			CHECK_MSTATUS_AND_RETURN_IT(status);

			if (argData.isFlagSet(COMMAND_INPUTS_FLAG))
				appendToResult((int)pInputElement.logicalIndex());
			if (argData.isFlagSet(COMMAND_OBJECTS_FLAG)) {
				MItDependencyGraph itGraph(pInputGeometry, MFn::kInvalid, MItDependencyGraph::kUpstream);

				for (itGraph.reset(); !itGraph.isDone(); itGraph.next()) {
					MObject node = itGraph.thisNode();

					if (node.apiType() != MFn::kMesh &&
						node.apiType() != MFn::kNurbsSurface &&
						node.apiType() != MFn::kSubdiv)
						continue;

					MFnDependencyNode fnSource(node, &status);
					CHECK_MSTATUS_AND_RETURN_IT(status);
					appendToResult(fnSource.name());
					break;
				}
			}
		}
	}
	// Query curves
	else if (!hasResult && argData.isFlagSet(COMMAND_CURVES_FLAG)) {
		MPlug pInput = fnNode.findPlug(CrossSectionsNode::aInput, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		MPlug pOutput = fnNode.findPlug(CrossSectionsNode::aOutput, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		if (pOutput.numElements() > 0) {
			status = createNode("transform", (fnTransform.name() + "_output").asChar(), MObject::kNullObj, m_oOutput);
			CHECK_MSTATUS_AND_RETURN_IT(status);

			for (unsigned int o = 0; o < argData.numberOfFlagUses(COMMAND_CURVES_FLAG); o++) {
				MArgList argList;
				argData.getFlagArgumentList(COMMAND_CURVES_FLAG, o, argList);
				int id = argList.asInt(0);

				MPlug pInputElement = pInput.elementByLogicalIndex(id, &status);
				CHECK_MSTATUS_AND_RETURN_IT(status);
				MPlug pOutputElement = pOutput.elementByLogicalIndex(id, &status);
				CHECK_MSTATUS_AND_RETURN_IT(status);

				MPlug pInputGeometry = pInputElement.child(CrossSectionsNode::aInputMesh, &status);
				CHECK_MSTATUS_AND_RETURN_IT(status);
				MPlug pOutputSections = pOutputElement.child(CrossSectionsNode::aOutputSections, &status);
				CHECK_MSTATUS_AND_RETURN_IT(status);

				MObject oOutputSections;
				if (pOutputSections.numElements() > 0) {
					MString name;

					MItDependencyGraph itGraph(pInputGeometry, MFn::kInvalid, MItDependencyGraph::kUpstream);
					for (itGraph.reset(); !itGraph.isDone(); itGraph.next()) {
						MObject node = itGraph.thisNode();

						if (node.apiType() != MFn::kMesh &&
							node.apiType() != MFn::kNurbsSurface &&
							node.apiType() != MFn::kSubdiv)
							continue;

						MFnDependencyNode fnSource(node, &status);
						CHECK_MSTATUS_AND_RETURN_IT(status);
						name = fnSource.name();
						break;
					}

					status = createNode("transform", (name + "_sections").asChar(), m_oOutput, oOutputSections);
					CHECK_MSTATUS_AND_RETURN_IT(status);

					for (unsigned int i = 0; i < pOutputSections.numElements(); i++) {
						MPlug pOutputSectionsElement = pOutputSections.elementByPhysicalIndex(i, &status);
						CHECK_MSTATUS_AND_RETURN_IT(status);

						MObject oSectionCurve;
						status = createNode("nurbsCurve", (name + "_section" + i).asChar(), oOutputSections, oSectionCurve);
						CHECK_MSTATUS_AND_RETURN_IT(status);
						MFnDagNode fnCurve(oSectionCurve, &status);
						CHECK_MSTATUS_AND_RETURN_IT(status);
						status = fnCurve.findPlug("create").setMObject(pOutputSectionsElement.asMObject());
						CHECK_MSTATUS_AND_RETURN_IT(status);
					}
				}
			}

			MTransformationMatrix tMatrix(fnTransform.transformationMatrix());
			MFnTransform fnOutput(m_oOutput);
			status = fnOutput.set(tMatrix);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			m_hasOutput = true;
		}

	}

	return MS::kSuccess;
}

MStatus CrossSectionsCommand::setFlagAttr(MArgDatabase& argData, char *flag, MPlug& attrPlug){
	MStatus status;

	for (unsigned int i = 0; i < ((attrPlug.isCompound()) ? attrPlug.numChildren() : 1) ; i++){
		MPlug pAttr = (attrPlug.isCompound()) ? attrPlug.child(i) : attrPlug;

		MDataHandle hAttr = pAttr.asMDataHandle();
		MFnNumericData::Type type = hAttr.numericType();

		switch (type) {
		case MFnNumericData::kBoolean: {
			bool value = argData.flagArgumentBool(flag, i, &status);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			status = m_dagMod.newPlugValueBool(pAttr, value);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			break; }
		case MFnNumericData::kInt: {
			int value = argData.flagArgumentInt(flag, i, &status);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			status = m_dagMod.newPlugValueInt(pAttr, value);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			break; }
		case MFnNumericData::kFloat: {
			float value = (float)argData.flagArgumentDouble(flag, i, &status);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			status = m_dagMod.newPlugValueFloat(pAttr, value);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			break; }
		case MFnNumericData::kDouble: {
			double value = argData.flagArgumentDouble(flag, i, &status);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			status = m_dagMod.newPlugValueDouble(pAttr, value);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			break; }
		default: {
			if (MString("t") == attrPlug.partialName()) {
				MDistance value = argData.flagArgumentMDistance(flag, i, &status);
				CHECK_MSTATUS_AND_RETURN_IT(status);
				status = m_dagMod.newPlugValueMDistance(pAttr, value);
				CHECK_MSTATUS_AND_RETURN_IT(status);
			}
			else if (MString("r") == attrPlug.partialName()) {
				MAngle value = argData.flagArgumentMAngle(flag, i, &status);
				CHECK_MSTATUS_AND_RETURN_IT(status);
				status = m_dagMod.newPlugValueMAngle(pAttr, value);
				CHECK_MSTATUS_AND_RETURN_IT(status);
			}
			break;}
		}
	}

	return MS::kSuccess;
}

MStatus CrossSectionsCommand::queryAttrValue(MPlug& attrPlug) {
	MStatus status;

	for (unsigned int i = 0; i < ((attrPlug.isCompound()) ? attrPlug.numChildren() : 1); i++) {
		MPlug pAttr = (attrPlug.isCompound()) ? attrPlug.child(i) : attrPlug;

		MDataHandle hAttr = pAttr.asMDataHandle();
		MFnNumericData::Type type = hAttr.numericType();

		switch (type) {
		case MFnNumericData::kBoolean:
			appendToResult(pAttr.asBool());
			break;
		case MFnNumericData::kInt:
			appendToResult(pAttr.asInt());
			break;
		case MFnNumericData::kFloat:
			appendToResult(pAttr.asFloat());
			break;
		case MFnNumericData::kDouble:
			appendToResult(pAttr.asDouble());
			break;
		default:
			if (MString("t") == attrPlug.partialName())
				appendToResult(pAttr.asMDistance().as(MDistance::uiUnit()));
			if (MString("r") == attrPlug.partialName())
				appendToResult(pAttr.asMAngle().as(MAngle::uiUnit()));
			break;
		}
	}

	return MS::kSuccess;
}