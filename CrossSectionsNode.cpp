#include "CrossSectionsNode.h"
#include "DistanceContext.h"
#include "pluginSetup.h"
#include "../_macros/macros.h"
#include "../_library/SCurvatureComb.h"
#include "../_library/SData.h"

#include <maya\MFnNumericAttribute.h>
#include <maya\MFnTypedAttribute.h>
#include <maya\MFnCompoundAttribute.h>
#include <maya\MFnMatrixAttribute.h>
#include <maya\MArrayDataHandle.h>
#include <maya\MPxManipContainer.h>
#include <maya\MFnDagNode.h>
#include <maya\MDagPath.h>
#include <maya\MFnMesh.h>
#include <maya\MFnNurbsSurface.h>
#include <maya\MFnNurbsCurve.h>
#include <maya\MFnNurbsCurveData.h>
#include <maya\MFnSubd.h>
#include <maya\MTimer.h>
#include <maya\MFnMeshData.h>
#include <maya\MGlobal.h>
#include <maya\MArrayDataBuilder.h>
#include <maya\MDistance.h>
#include <maya\MIntArray.h>
#include <maya\MDagModifier.h>
#include <maya\MFloatPointArray.h>
#include <maya\MPlugArray.h>
#include <maya\MUiMessage.h>
#include <maya\MDagPath.h>
#include <maya\MItDependencyNodes.h>
#include <maya\MCommandResult.h>
#include <maya\MDGMessage.h>

MTypeId CrossSectionsNode::id(NODE_ID);
MString CrossSectionsNode::drawDbNODE_CLASSIFICATION(NODE_CLASSIFICATION);
MString CrossSectionsNode::drawRegistrantId(NODE_REGISTRANT_ID);

MObject CrossSectionsNode::aOutput;
MObject CrossSectionsNode::aOutputSections;

MObject	CrossSectionsNode::aNodeMatrix;
MObject CrossSectionsNode::aNumberOfPlanes;
MObject CrossSectionsNode::aPlaneSpacing;
MObject CrossSectionsNode::aLineWidth;
MObject CrossSectionsNode::aVisualClip;

MObject	CrossSectionsNode::aCurvatureComb;
MObject	CrossSectionsNode::aCombSamples;
MObject	CrossSectionsNode::aCombScale;
MObject	CrossSectionsNode::aCombSmooth;
MObject	CrossSectionsNode::aCombSmoothRadius;

MObject CrossSectionsNode::aInput;
MObject CrossSectionsNode::aInputMesh;
MObject CrossSectionsNode::aInputMatrix;
MObject CrossSectionsNode::aInputColor;

MObject CrossSectionsNode::activeClipPlane;

CrossSectionsNode::CrossSectionsNode() {
}

void CrossSectionsNode::postConstructor() {
	MStatus status;

	m_initialize = true;
	status = setExistWithoutInConnections(false);
	CHECK_MSTATUS(status);

	MDagPath path;
	MFnDagNode fnNode(thisMObject(), &status);
	CHECK_MSTATUS(status);
	status = fnNode.getPath(path);
	CHECK_MSTATUS(status);
	
	m_callbackIDs.append(int(MDagMessage::addParentAddedDagPathCallback(path, parentChanged, NULL, &status)) );
	CHECK_MSTATUS(status);
	m_callbackIDs.append(int(MDagMessage::addParentRemovedDagPathCallback(path, parentChanged, NULL, &status)) );
	CHECK_MSTATUS(status);
	m_callbackIDs.append(int(MNodeMessage::addAttributeChangedCallback(thisMObject(), attributeChanged, NULL, &status)));
	CHECK_MSTATUS(status);
	m_callbackIDs.append(int(MDGMessage::addConnectionCallback(connectionChanged, NULL, &status)));
	CHECK_MSTATUS(status);
}

CrossSectionsNode::~CrossSectionsNode() {
	MMessage::removeCallbacks(m_callbackIDs);
}

void* CrossSectionsNode::creator(){
	return new CrossSectionsNode();
}

MStatus CrossSectionsNode::initialize() {
	MStatus status;

	MFnNumericAttribute nAttr;
	MFnTypedAttribute tAttr;
	MFnCompoundAttribute cAttr;
	MFnMatrixAttribute mAttr;

	// Output
	aOutput = cAttr.create("output", "output", &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	cAttr.setWritable(false);
	cAttr.setArray(true);
	cAttr.setUsesArrayDataBuilder(true);

	aOutputSections = tAttr.create("outputSections", "outputSections", MFnData::kNurbsCurve, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	tAttr.setWritable(false);
	tAttr.setArray(true);
	tAttr.setUsesArrayDataBuilder(true);
	ADD_ATTRIBUTE(aOutputSections);
	cAttr.addChild(aOutputSections);

	ADD_ATTRIBUTE(aOutput);

	// Numeric
	aNodeMatrix = mAttr.create("nodeMatrix", "nodeMatrix", MFnMatrixAttribute::kDouble, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	mAttr.setHidden(true);
	ADD_ATTRIBUTE(aNodeMatrix);
	ATTRIBUTE_AFFECTS(aNodeMatrix, aOutput);

	MAKE_NUMERIC_ATTR(aNumberOfPlanes, "numberOfPlanes", MFnNumericData::kInt, false, 1, 1, 10);
	ATTRIBUTE_AFFECTS(aNumberOfPlanes, aOutput);

	MDistance distance;
	MAKE_NUMERIC_ATTR(aPlaneSpacing, "planeSpacing", MFnNumericData::kDouble, false, distance.internalToUI(10), 0, distance.internalToUI(100));
	ATTRIBUTE_AFFECTS(aPlaneSpacing, aOutput);

	MAKE_NUMERIC_ATTR(aLineWidth, "lineWidth", MFnNumericData::kFloat, false, 2, 1, 10);
	ATTRIBUTE_AFFECTS(aLineWidth, aOutput);

	aVisualClip = nAttr.create("visualClip", "visualClip", MFnNumericData::kBoolean, false, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	ADD_ATTRIBUTE(aVisualClip);
	ATTRIBUTE_AFFECTS(aVisualClip, aOutput);

	// Curvature
	aCurvatureComb = nAttr.create("curvatureComb", "curvatureComb", MFnNumericData::kBoolean, false, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	ADD_ATTRIBUTE(aCurvatureComb);
	ATTRIBUTE_AFFECTS(aCurvatureComb, aOutput);

	MAKE_NUMERIC_ATTR(aCombSamples, "combSamples", MFnNumericData::kInt, false, 75, 3, 200);
	ATTRIBUTE_AFFECTS(aCombSamples, aOutput);

	MAKE_NUMERIC_ATTR(aCombScale, "combScale", MFnNumericData::kFloat, false, 100, 0, 1000);
	ATTRIBUTE_AFFECTS(aCombScale, aOutput);

	aCombSmooth = nAttr.create("combSmooth", "combSmooth", MFnNumericData::kBoolean, true, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	ADD_ATTRIBUTE(aCombSmooth);
	ATTRIBUTE_AFFECTS(aCombSmooth, aOutput);

	MAKE_NUMERIC_ATTR(aCombSmoothRadius, "combSmoothRadius", MFnNumericData::kInt, false, 2, 1, 10);
	ATTRIBUTE_AFFECTS(aCombSmoothRadius, aOutput);

	// Input
	aInput = cAttr.create("input", "input", &status);
	cAttr.setArray(true);
	cAttr.setDisconnectBehavior(MFnAttribute::kDelete);

	MAKE_TYPED_ATTR(aInputMesh, "inputMesh", MFnData::kMesh, MObject::kNullObj, true);
	ATTRIBUTE_AFFECTS(aInputMesh, aOutput);
	cAttr.addChild(aInputMesh);

	aInputMatrix = mAttr.create("inputMatrix", "inputMatrix", MFnMatrixAttribute::kDouble, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	mAttr.setHidden(true);
	mAttr.setDisconnectBehavior(MFnAttribute::kDelete);
	ADD_ATTRIBUTE(aInputMatrix);
	ATTRIBUTE_AFFECTS(aInputMatrix, aOutput);
	cAttr.addChild(aInputMatrix);

	aInputColor = nAttr.createColor("inputColor", "inputColor", &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	nAttr.setDefault(1.0f, 1.0f, 0.0f);
	nAttr.setDisconnectBehavior(MFnAttribute::kDelete);
	ADD_ATTRIBUTE(aInputColor);
	ATTRIBUTE_AFFECTS(aInputColor, aOutput);
	cAttr.addChild(aInputColor);

	ADD_ATTRIBUTE(aInput);
	ATTRIBUTE_AFFECTS(aInput, aOutput);

	MPxManipContainer::addToManipConnectTable(id);

	return MS::kSuccess;
}

MStatus CrossSectionsNode::setDependentsDirty(const MPlug &plug, MPlugArray &plugArray) {
	MStatus status;

	// Numeric
	if (plug == aNodeMatrix ||
		plug == aNumberOfPlanes ||
		plug == aPlaneSpacing)
		m_dirtyNode = true;

	// Display
	if (plug == aLineWidth)
		m_dirtyLineWidth = true;

	// Comb
	if (plug == aCurvatureComb ||
		plug == aCombSamples ||
		plug == aCombScale ||
		plug == aCombSmooth ||
		plug == aCombSmoothRadius)
		m_dirtyComb = true;

	// Input
	if (plug == aInputMesh)
		m_dirtyInput =
		m_dirtyGeometry[plug.parent().logicalIndex()] =
		m_dirtyCurvature[plug.parent().logicalIndex()] =
		m_dirtySections[plug.parent().logicalIndex()] = true;

	if (plug == aInputMatrix)
		m_dirtyInput =
		m_dirtyMatrix[plug.parent().logicalIndex()] =
		m_dirtyCurvature[plug.parent().logicalIndex()] =
		m_dirtySections[plug.parent().logicalIndex()] = true;

	if (plug == aInputColor)
		m_dirtyInput =
		m_dirtyColor[plug.parent().logicalIndex()] = true;

	if (plug == aInput)
		m_dirtyInput = true;

	CHECK_MSTATUS_AND_RETURN_IT(status);

	return MS::kSuccess;
}

MStatus CrossSectionsNode::compute(const MPlug &plug, MDataBlock &dataBlock) {
	MStatus status;

	// Load attributes ////////////////////////////////////////////////////////////////////////////
	if (plug != aOutput)
		return MS::kSuccess;

	// Update numeric attributes
	if (m_dirtyNode || m_initialize) {
		m_data.matrixInverse = dataBlock.inputValue(aNodeMatrix, &status).asMatrix().inverse();
		CHECK_MSTATUS_AND_RETURN_IT(status);
		m_data.numberOfPlanes = dataBlock.inputValue(aNumberOfPlanes, &status).asInt();
		CHECK_MSTATUS_AND_RETURN_IT(status);
		MDistance conversion;
		m_data.planeSpacing = conversion.uiToInternal(dataBlock.inputValue(aPlaneSpacing, &status).asDouble());
		CHECK_MSTATUS_AND_RETURN_IT(status);
	}

	if (m_dirtyLineWidth || m_initialize) {
		m_dirtyLineWidth = false;
		m_data.lineWidth = dataBlock.inputValue(aLineWidth, &status).asFloat();
		CHECK_MSTATUS_AND_RETURN_IT(status);
	}

	// Update curvature attr
	if (m_dirtyComb || m_initialize) {
		m_data.curvatureComb = dataBlock.inputValue(aCurvatureComb, &status).asBool();
		CHECK_MSTATUS_AND_RETURN_IT(status);
		m_data.crvSamples = dataBlock.inputValue(aCombSamples, &status).asInt();
		CHECK_MSTATUS_AND_RETURN_IT(status);
		m_data.crvScale = dataBlock.inputValue(aCombScale, &status).asFloat();
		CHECK_MSTATUS_AND_RETURN_IT(status);
		m_data.crvFilter = dataBlock.inputValue(aCombSmooth, &status).asBool();
		CHECK_MSTATUS_AND_RETURN_IT(status);
		m_data.crvRadius = dataBlock.inputValue(aCombSmoothRadius, &status).asInt();
		CHECK_MSTATUS_AND_RETURN_IT(status);
	}

	// Update input attribute /////////////////////////////////////////////////////////////////////
	if (m_dirtyNode || m_dirtyInput || m_dirtyComb || m_initialize) {

		// Array handle
		MArrayDataHandle hInput = dataBlock.inputArrayValue(aInput, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		unsigned int numInputs = hInput.elementCount(&status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		for (unsigned int i = 0; i < numInputs; i++) {
			status = hInput.jumpToArrayElement(i);
			unsigned int logicalIndex = hInput.elementIndex(&status);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			MDataHandle hInputElement = hInput.inputValue(&status);
			CHECK_MSTATUS_AND_RETURN_IT(status);

			// Update color
			if (m_dirtyColor[logicalIndex] || m_dirtyGeometry[logicalIndex] || m_initialize) {
				m_dirtyColor[logicalIndex] = false;
				MDataHandle hInputColor = hInputElement.child(aInputColor);
				m_data.inputColor[logicalIndex] = MColor(hInputColor.asFloat3());
			}
			// Update matrix
			if (m_dirtyMatrix[logicalIndex] || m_initialize) {
				m_dirtyMatrix[logicalIndex] = false;
				MDataHandle hInputMatrix = hInputElement.child(aInputMatrix);
				m_data.inputMatrix[logicalIndex] = hInputMatrix.asMatrix();
			}
			// Update mesh
			if (m_dirtyGeometry[logicalIndex] || m_initialize) {
				m_dirtyGeometry[logicalIndex] = false;
				MDataHandle hInputGeometry = hInputElement.child(aInputMesh);
				m_data.inputGeometry[logicalIndex] = hInputGeometry.asMesh();
				CHECK_MSTATUS_AND_RETURN_IT(status);
			}
			// Update sections
			if (m_dirtySections[logicalIndex] || m_initialize || m_dirtyNode) {
				m_dirtySections[logicalIndex] = false;
				status = m_data.sectionCurves[logicalIndex].clear();
				CHECK_MSTATUS_AND_RETURN_IT(status);

				if (m_data.inputGeometry[logicalIndex].isNull())
					continue;

				MMatrix currentMatrix = m_data.inputMatrix[logicalIndex] * m_data.matrixInverse;
				status = generateMeshSections(m_data.inputGeometry[logicalIndex], currentMatrix, m_data.sectionCurves[logicalIndex]);
				CHECK_MSTATUS_AND_RETURN_IT(status);
			}
			// Update curvature comb
			if (m_data.curvatureComb && (m_dirtyCurvature[logicalIndex] || m_initialize || m_dirtyNode || m_dirtyComb)) {
				m_dirtyCurvature[logicalIndex] = false;
				m_data.samplePoints.erase(logicalIndex);
				m_data.crvPoints.erase(logicalIndex);

				for (unsigned int c = 0; c < m_data.sectionCurves[logicalIndex].length(); c++) {
					SCurvatureComb fnComb(m_data.sectionCurves[logicalIndex][c], m_data.crvSamples, m_data.crvScale, &status);
					CHECK_MSTATUS_AND_RETURN_IT(status);

					if (m_data.crvFilter)
						fnComb.setFilter(m_data.crvRadius);

					status = fnComb.getPoints(m_data.samplePoints[logicalIndex][c], m_data.crvPoints[logicalIndex][c]);
					CHECK_MSTATUS_AND_RETURN_IT(status);
				}
			}
		}
		status = hInput.setAllClean();
		CHECK_MSTATUS_AND_RETURN_IT(status);

		if (NULL != m_data.ctxPtr)
			m_data.ctxPtr->updateManip();

		m_dirtyInput =
		m_dirtyNode =
		m_dirtyComb = false;
	}

	// Write curves in output attribute //////////////////////////////////////////////////////////////
	MArrayDataHandle hOutput = dataBlock.outputArrayValue(aOutput, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	MArrayDataBuilder bOutput(&dataBlock, aOutput, (int)m_data.sectionCurves.size(), &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Add element for each mesh
	for (auto const& element : m_data.sectionCurves) {
		unsigned int index = element.first;

		MDataHandle hOutputElement = bOutput.addElement(index, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		hOutputElement = hOutputElement.child(aOutputSections);
		MArrayDataHandle hOutputCurves(hOutputElement);
		MArrayDataBuilder bOutputElement(&dataBlock, aOutputSections, m_data.sectionCurves[index].length(), &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		// Add section curves for current mesh
		for (unsigned int c = 0; c < m_data.sectionCurves[index].length(); c++) {
			MDataHandle hOutputCurve = bOutputElement.addElement(c, &status);
			CHECK_MSTATUS_AND_RETURN_IT(status);
			status = hOutputCurve.set(m_data.sectionCurves[index][c]);
			CHECK_MSTATUS_AND_RETURN_IT(status);
		}

		status = hOutputCurves.set(bOutputElement);
		CHECK_MSTATUS_AND_RETURN_IT(status);
	}

	status = hOutput.set(bOutput);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = hOutput.setAllClean();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	status = dataBlock.setClean(aOutput);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	if(m_initialize)
		m_initialize = false;

	return MS::kSuccess;
}

bool CrossSectionsNode::isBounded() const {
	return false;
}

MBoundingBox CrossSectionsNode::boundingBox() const {
	MStatus status;
	MBoundingBox bBox;
	bBox.transformUsing(m_data.matrixInverse);
	bBox.expand(MPoint(-1, -1, -1));
	bBox.expand(MPoint(1, 1, 1));

	for (auto const& element : m_data.sectionCurves) {
		for (unsigned int i = 0; i < element.second.length(); i++) {
			MFnNurbsCurve fnCurve(element.second[i], &status);
			CHECK_MSTATUS(status);
			MPointArray curvePoints;
			status = fnCurve.getCVs(curvePoints);
			CHECK_MSTATUS(status);

			for (unsigned int i = 0; i < curvePoints.length(); i++)
				bBox.expand(curvePoints[i]);
		}
	}

	return bBox;
}

void CrossSectionsNode::draw(M3dView &view, const MDagPath &path, M3dView::DisplayStyle style, M3dView::DisplayStatus stat){
	MStatus status;
	
	// Query output attribute to force compute
	MFnDagNode fnNode(thisMObject());
	MPlug pSection = fnNode.findPlug(aOutput, &status);
	CHECK_MSTATUS(status);
	pSection.asMObject();

	view.beginGL();

	if(!activeClipPlane.isNull() && activeClipPlane != thisMObject())
		glEnable(GL_CLIP_PLANE6);
	else
		glDisable(GL_CLIP_PLANE6);

	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glDepthFunc(GL_LEQUAL);
	glLineWidth(m_data.lineWidth);

	// Draw sections
	for (auto const& element : m_data.sectionCurves) {
		glColor3f(m_data.inputColor[element.first].r, m_data.inputColor[element.first].g, m_data.inputColor[element.first].b);

		for (unsigned int i = 0; i < element.second.length(); i++) {
			MFnNurbsCurve fnCurve(element.second[i], &status);
			CHECK_MSTATUS(status);
			MPointArray curvePoints;
			status = fnCurve.getCVs(curvePoints);
			CHECK_MSTATUS(status);

			glBegin(GL_LINE_STRIP);
			for (unsigned int i = 0; i < curvePoints.length(); i++)
				glVertex3d(curvePoints[i].x, curvePoints[i].y, curvePoints[i].z);
			glEnd();
		}
	}

	// Draw comb
	if (m_data.curvatureComb) {
		glColor3f(0, 0.2f, 0);
		glLineWidth(1.0f);

		for (auto const& element : m_data.crvPoints) {
			for (auto const& curve : element.second) {
				glBegin(GL_LINE_STRIP);
				for (unsigned int i = 0; i < curve.second.length(); i++)
					glVertex3d(curve.second[i].x, curve.second[i].y, curve.second[i].z);
				glEnd();
			}
		}

		glColor3f(1.0f, 0, 0);
		glBegin(GL_LINES);
		for (auto const& element : m_data.crvPoints) {
			for (auto const& curve : element.second) {
				for (unsigned int i = 0; i < curve.second.length(); i++) {
					glVertex3d(curve.second[i].x, curve.second[i].y, curve.second[i].z);
					glVertex3d(m_data.samplePoints[element.first][curve.first][i].x, m_data.samplePoints[element.first][curve.first][i].y, m_data.samplePoints[element.first][curve.first][i].z);
				}
			}
		}
		glEnd();
	}

	glPopAttrib();

	glDisable(GL_CLIP_PLANE6);

	view.endGL();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Data	 //////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

CrossSectionsData* CrossSectionsNode::getData() {
	return &m_data;
}

void CrossSectionsNode::clearIndex(unsigned int index) {
	m_dirtyGeometry.erase(index);
	m_dirtySections.erase(index);
	m_dirtyMatrix.erase(index);
	m_dirtyColor.erase(index);

	m_data.inputGeometry.erase(index);
	m_data.inputMatrix.erase(index);
	m_data.inputGeometry.erase(index);
	m_data.sectionCurves.erase(index);

	m_data.samplePoints.erase(index);
	m_data.crvPoints.erase(index);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Callbacks //////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CrossSectionsNode::parentChanged(MDagPath &child, MDagPath &parent, void *data) {
	MStatus status;

	MFnDagNode fnNode(child);
	if (child.transform().isNull())
		return;

	// Always connect parents world matrix to the plugin node
	MFnDagNode fnTransform(child.transform(), &status);
	MPlug pWorldMatrix = fnTransform.findPlug("worldMatrix", &status);
	CHECK_MSTATUS(status);
	pWorldMatrix = pWorldMatrix.elementByLogicalIndex(0, &status);
	CHECK_MSTATUS(status);
	MPlug pNodeMatrix = fnNode.findPlug(CrossSectionsNode::aNodeMatrix, &status);
	CHECK_MSTATUS(status);
	MDagModifier dagMod;
	status = dagMod.connect(pWorldMatrix, pNodeMatrix);
	CHECK_MSTATUS(status);
	status = dagMod.doIt();
	CHECK_MSTATUS(status);
}

void CrossSectionsNode::attributeChanged(MNodeMessage::AttributeMessage msg, MPlug & plug, MPlug & otherPlug, void *data) {
	MStatus status;

	// Set visual clip	
	if (aVisualClip == plug) {
		MFnDependencyNode fnNode(plug.node());
		
		bool clipValue;
		status = plug.getValue(clipValue);
		CHECK_MSTATUS(status);

		if (clipValue){
			// Set active plane
			activeClipPlane = plug.node();

			// Deactivate other visual clips
			MItDependencyNodes itNodes(MFn::kPluginLocatorNode, &status);
			CHECK_MSTATUS(status);

			for (itNodes.reset(); !itNodes.isDone(); itNodes.next()) {
				if (plug.node() == itNodes.thisNode())
					continue;

				MFnDependencyNode fnNode(itNodes.thisNode(), &status);
				CHECK_MSTATUS(status);

				if (fnNode.typeId() != CrossSectionsNode::id)
					continue;

				MPlug pClip = fnNode.findPlug(CrossSectionsNode::aVisualClip, &status);
				CHECK_MSTATUS(status);

				pClip.setBool(false);
			}

			M3dView activeView = M3dView::active3dView();
			if (activeView.getRendererName(&status) == M3dView::kViewport2Renderer)
				MGlobal::displayWarning("Visual clipping is not supported in Viewport 2.0");
		}
		// Deactivate active visual clip
		else if(activeClipPlane == plug.node())
			activeClipPlane = MObject::kNullObj;
	}

}

void CrossSectionsNode::connectionChanged(MPlug &srcPlug, MPlug &destPlug, bool made, void *clientData) {
	MStatus status;

	if (made)
		return;

	// Delete helper node
	if (destPlug == aInputMesh) {
		MObject srcNode = srcPlug.node();
		if (!srcNode.isNull() && srcNode.apiType() != MFn::kMesh) {
			MFnDependencyNode fnNode(srcNode, &status);
			CHECK_MSTATUS(status);
			status = MGlobal::executeCommand("if( `objExists "+fnNode.name()+"` ) delete " + fnNode.name(), false, true);
			CHECK_MSTATUS(status);
		}
	}

	// Delete input array after connection broken
	if (destPlug.parent() == aInput) {
		status = MGlobal::executeCommand("removeMultiInstance -b true " + destPlug.parent().name(), false, true);
		CHECK_MSTATUS(status);

		MFnDependencyNode fnNode(destPlug.node(), &status);
		CHECK_MSTATUS(status);
		CrossSectionsNode *nodePtr = dynamic_cast <CrossSectionsNode*>(fnNode.userNode());
		nodePtr->clearIndex(destPlug.parent().logicalIndex());
	}

	// Reconnect matrix plug to parent node
	if (aNodeMatrix == destPlug) {
		MFnDagNode fnNode(destPlug.node());
		MDagPath path;
		status = fnNode.getPath(path);
		CHECK_MSTATUS(status);
		parentChanged(path, path, NULL);
	}

}

void CrossSectionsNode::preRender(const MString &panel, void *data){
	MStatus status;

	if (CrossSectionsNode::activeClipPlane.isNull())
		return;

	M3dView activeView = M3dView::active3dView();
	MDagPath activeCamera;
	activeView.getCamera(activeCamera);

	// Get modelview matrix
	M3dView view;
	status = M3dView::getM3dViewFromModelPanel(panel, view);
	CHECK_MSTATUS(status);

	MDagPath cam;
	view.getCamera(cam);
	MMatrix cMatrix = cam.inclusiveMatrixInverse();

	// Perform clipping only in the active view
	if (cam.fullPathName() != activeCamera.fullPathName()  || M3dView::kViewport2Renderer == view.getRendererName(&status))
		return;

	// Get active plane transformation matrix
	MFnDagNode fnDagNode(CrossSectionsNode::activeClipPlane, &status);
	if (MS::kSuccess != status)
		return;

	MDagPath path;
	status = fnDagNode.getPath(path);
	CHECK_MSTATUS(status);
	MMatrix planeMatrix = path.inclusiveMatrix();

	M3dView::DisplayObjects mask = static_cast<M3dView::DisplayObjects>(view.objectDisplay());
	bool showGrid = (0 != (mask & M3dView::kDisplayGrid)) ? true : false;
	
	// Open GL ///////////////////////////////////////////////////////////////////////////////////////
	view.beginGL();
	glPushMatrix();

	// Set clipping plane matrix
	planeMatrix *= cMatrix;
	GLdouble modelview[16], planeview[16];
	for (unsigned int i = 0; i < 16; i++) {
		planeview[i] = planeMatrix[(int)floor((double)i / 4)][i % 4];
		modelview[i] = cMatrix[(int)floor((double)i / 4)][i % 4];
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(planeview);

	GLdouble eqn[4] = { -1, 0, 0, 0 };
	glClipPlane(GL_CLIP_PLANE6, eqn);
	glEnable(GL_CLIP_PLANE6);

	glLoadMatrixd(modelview);

	glPushAttrib(GL_ALL_ATTRIB_BITS);

	if (showGrid) {
		double
			spacing,
			size;
		int
			divisions,
			displayAxes,
			displayGridLines,
			displayDivisionLines,
			displayAxesBold,
			grid,
			gridHighlight,
			gridAxes;

		// Grid settings
		MGlobal::executeCommand("grid -q -spacing", spacing);
		MGlobal::executeCommand("grid -q -size", size);
		MGlobal::executeCommand("grid -q -divisions", divisions);
		MGlobal::executeCommand("grid -q -displayAxes", displayAxes );
		MGlobal::executeCommand("grid -q -displayGridLines", displayGridLines );
		MGlobal::executeCommand("grid -q -displayDivisionLines", displayDivisionLines );
		MGlobal::executeCommand("grid -q -displayAxesBold", displayAxesBold );
		
		// Color settings
		MGlobal::executeCommand("displayColor -q -dormant \"grid\"", grid);
		MGlobal::executeCommand("displayColor -q -dormant \"gridHighlight\"", gridHighlight);
		MGlobal::executeCommand("displayColor -q -dormant \"gridAxis\"", gridAxes);

		MColor cGrid = view.colorAtIndex(grid - 1, M3dView::kDormantColors);
		MColor cGridHighlight = view.colorAtIndex(gridHighlight - 1, M3dView::kDormantColors);
		MColor cGridAxes = view.colorAtIndex(gridAxes - 1, M3dView::kDormantColors);

		spacing = MDistance::uiToInternal(spacing);
		size = MDistance::uiToInternal(size);

		// Draw grid and divisions
		glBegin(GL_LINES);
		double step = spacing / divisions;
		for(unsigned int i=1; i <= ceil(size/ step); i++){
			double position = (size > i*step) ? i*step : size;

			if (0==i%divisions)
				glColor3d(cGridHighlight.r, cGridHighlight.g, cGridHighlight.b);
			else
				glColor3d(cGrid.r, cGrid.g, cGrid.b);

			if (0 == i%divisions && 0 == displayGridLines)
				continue;
			if (0 != i%divisions && 0 == displayDivisionLines && position!=size)
				continue;

			glVertex3d(-size, position, 0);
			glVertex3d(size, position, 0);

			glVertex3d(-size, -position, 0);
			glVertex3d(size, -position, 0);

			glVertex3d(-position, -size, 0);
			glVertex3d(-position, size, 0);

			glVertex3d(position, -size, 0);
			glVertex3d(position, size, 0);
		}
		glEnd();

		// Draw axes
		if (1 == displayAxes) {
			glColor3d(cGridAxes.r, cGridAxes.g, cGridAxes.b);

			if(1 == displayAxesBold)
				glLineWidth(2.0f);

			glBegin(GL_LINES);
			glVertex3d(-size, 0, 0);
			glVertex3d(size, 0, 0);

			glVertex3d(0, -size, 0);
			glVertex3d(0, size, 0);
			glEnd();
		}
		
	}

	glPopAttrib();

	glLoadMatrixd(planeview);

	eqn[0] = 1;
	glClipPlane(GL_CLIP_PLANE6, eqn);

	glPopMatrix();
	view.endGL();
}

void CrossSectionsNode::postRender(const MString &panel, void *data) {
	MStatus status;

	M3dView view;
	status = M3dView::getM3dViewFromModelPanel(panel, view);
	CHECK_MSTATUS(status);

	view.beginGL();

	glDisable(GL_CLIP_PLANE6);

	view.endGL();
}

void CrossSectionsNode::nodeDeleted(MObject &node, void *data) {
	MStatus status;

	if (activeClipPlane == node)
		activeClipPlane = MObject::kNullObj;
}

void CrossSectionsNode::meshCreated(MObject &node, void *data) {
	MStatus status;

	if (!SData::isMesh(node))
		return;

	MFnDagNode fnMesh(node);
	fnMesh.findPlug("useGlobalSmoothDrawType").setBool(false);
	fnMesh.findPlug("smoothDrawType").setInt(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Intersection ///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

MStatus CrossSectionsNode::generateMeshSections(MObject& mesh, MMatrix& matrix, MObjectArray& sectionCurves) {
	MStatus status;

	if (!SData::isMesh(mesh))
		return MS::kInvalidParameter;

	for (unsigned int i = 0; i < m_data.numberOfPlanes; i++) {
		SSectionPlane sectionPlane(MPoint(m_data.planeSpacing*i, 0, 0), MVector::xAxis);
		MObjectArray curves;
		status = sectionPlane.getIntersections(mesh, curves, matrix);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		for (unsigned int i = 0; i < curves.length(); i++)
			sectionCurves.append(curves[i]);
	}

	return MS::kSuccess;
}