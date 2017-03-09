#pragma once
#include "pluginSetup.h"
#include "CrossSectionsNode.h"

#include <maya\MPxCommand.h>
#include <maya\MDagModifier.h>
#include <maya\MDagPath.h>
#include <maya\MPlug.h>
#include <maya\MDistance.h>
#include <maya\MAngle.h>
#include <maya\MArgDatabase.h>
#include <maya\MDagPathArray.h>
#include <maya\MBoundingBox.h>
#include <map>

class CrossSectionsCommand : public MPxCommand{
public:
	CrossSectionsCommand();
	virtual ~CrossSectionsCommand();
	static void*	creator();
	static MSyntax	newSyntax();

	virtual MStatus	doIt(const MArgList& args);
	virtual MStatus	redoIt();
	virtual MStatus	undoIt();

	virtual bool	isUndoable() const;

private:
	std::map <char*, MObject> m_attrFlags{
		{COMMAND_NUM_PLANES_FLAG,	CrossSectionsNode::aNumberOfPlanes},
		{COMMAND_SPACING_FLAG,		CrossSectionsNode::aPlaneSpacing},
		{COMMAND_LINEWIDTH_FLAG,	CrossSectionsNode::aLineWidth},
		{COMMAND_COMB_FLAG,			CrossSectionsNode::aCurvatureComb},
		{COMMAND_SAMPLES_FLAG,		CrossSectionsNode::aCombSamples},
		{COMMAND_SCALE_FLAG,		CrossSectionsNode::aCombScale},
		{COMMAND_SMOOTH_FLAG,		CrossSectionsNode::aCombSmooth},
		{COMMAND_RADIUS_FLAG,		CrossSectionsNode::aCombSmoothRadius},
		{COMMAND_CLIP_FLAG,			CrossSectionsNode::aVisualClip},
		{COMMAND_ROTATE_FLAG,		MObject::kNullObj },
		{COMMAND_TRANSLATE_FLAG,	MObject::kNullObj }
	};

	MStatus createPluginNode(MString& name);
	MStatus getPluginNode(MString& name);
	bool	nodeIsConnectedToInput(MDagPath& path);
	
	MStatus getShapes(MDagPath& path, MDagPathArray& shapes);
	MStatus filterSelection(MSelectionList& selection, MDagPathArray& filteredNodes);

	MStatus connectGeometry(MDagPathArray& pathArray);
	MStatus connectGeometry(MDagPath& path, MPlug& pInputElementPlug);

	MPlug findWorldMatrixPlug(MDagPath& path, MStatus *status);
	MStatus findFreeInputPlug(MPlug& inputPlug, unsigned int &index);
	
	MStatus setAttributes(MArgDatabase& argData);
	MStatus setFlagAttr(MArgDatabase& argData, char *flag, MPlug& attrPlug);
	MStatus queryAttrValue(MPlug& attrPlug);
	MStatus queryAttributes(MArgDatabase& argData);
	MStatus createNode(const char *type, const char *name, MObject& parent, MObject& node);

	MDagModifier	m_dagMod;
	MObject			m_oNode,
					m_oTransform,
					m_oOutput;

	MBoundingBox	m_bBox;
	bool	m_useBBox = false;

	bool	m_isEdit,
			m_isQuery,
			m_isCreation;
	bool	m_hasOutput = false;
};