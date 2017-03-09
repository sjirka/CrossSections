#include <maya\MFnPlugin.h>
#include <maya\MDrawRegistry.h>
#include <maya\MUiMessage.h>
#include <maya\MCallbackIdArray.h>
#include <maya\MDGMessage.h>

#include "pluginSetup.h"
#include "CrossSectionsNode.h"
#include "CrossSectionsCommand.h"
#include "CrossSectionsDrawOverride.h"
#include "CrossSectionsManipContainer.h"
#include "CrossSectionsPlaneManip.h"
#include "DistanceCmd.h"
#include "DistanceToolCmd.h"
#include "DistanceManipContainer.h"
#include "DistanceSectionManip.h"

MCallbackIdArray CrossSectionsNode::clipCallbacks;

MStatus initializePlugin(MObject object) {
	MStatus status;

	MFnPlugin fnPlugin(object, "Stepan Jirka", "2.0", "Any", &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Plugin node ////////////////////////////////////////////////////////////////////////////////
	status = fnPlugin.registerNode(
		NODE_NAME,
		CrossSectionsNode::id,
		CrossSectionsNode::creator,
		CrossSectionsNode::initialize,
		MPxNode::kLocatorNode,
		&CrossSectionsNode::drawDbNODE_CLASSIFICATION);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Draw override
	status = MHWRender::MDrawRegistry::registerDrawOverrideCreator(
		CrossSectionsNode::drawDbNODE_CLASSIFICATION,
		CrossSectionsNode::drawRegistrantId,
		CrossSectionsDrawOverride::Creator);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Node command
	status = fnPlugin.registerCommand(
		COMMAND_NAME,
		CrossSectionsCommand::creator,
		CrossSectionsCommand::newSyntax);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Node manip container
	status = fnPlugin.registerNode(
		MANIP_CONTAINER_NAME,
		CrossSectionsManipContainer::id,
		CrossSectionsManipContainer::creator,
		CrossSectionsManipContainer::initialize,
		MPxNode::kManipContainer);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Node manip
	status = fnPlugin.registerNode(
		PLANE_MANIP_NAME,
		CrossSectionsPlaneManip::id,
		CrossSectionsPlaneManip::creator,
		CrossSectionsPlaneManip::initialize,
		MPxNode::kManipulatorNode);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Distance context ///////////////////////////////////////////////////////////////////////////
	status = fnPlugin.registerContextCommand(
		DISTANCE_CONTEXT_NAME,
		DistanceCmd::creator,
		DISTANCE_COMMAND_NAME,
		DistanceToolCmd::creator);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Distance manip container
	status = fnPlugin.registerNode(
		DISTANCE_CONTAINER_NAME,
		DistanceManipContainer::id,
		DistanceManipContainer::creator,
		DistanceManipContainer::initialize,
		MPxNode::kManipContainer);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Distance manip
	status = fnPlugin.registerNode(
		CURVE_MANIP_NAME,
		DistanceSectionManip::id,
		DistanceSectionManip::creator,
		DistanceSectionManip::initialize,
		MPxNode::kManipulatorNode);
	CHECK_MSTATUS_AND_RETURN_IT(status);



	MStringArray panels;
	status = MGlobal::executeCommand("getPanel -type \"modelPanel\"", panels);

	for (unsigned int i = 0; i < panels.length(); i++) {
		CrossSectionsNode::clipCallbacks.append(MUiMessage::add3dViewPreRenderMsgCallback(panels[i], CrossSectionsNode::preRender, NULL, &status));
		CHECK_MSTATUS_AND_RETURN_IT(status);
		CrossSectionsNode::clipCallbacks.append(MUiMessage::add3dViewPostRenderMsgCallback(panels[i], CrossSectionsNode::postRender, NULL, &status));
		CHECK_MSTATUS_AND_RETURN_IT(status);
	}
	CrossSectionsNode::clipCallbacks.append(MDGMessage::addNodeRemovedCallback(CrossSectionsNode::nodeDeleted, NODE_NAME, NULL, &status));
	CHECK_MSTATUS_AND_RETURN_IT(status);
	CrossSectionsNode::clipCallbacks.append(MDGMessage::addNodeAddedCallback(CrossSectionsNode::meshCreated, "mesh", NULL, &status));
	CHECK_MSTATUS_AND_RETURN_IT(status);
	
	return MS::kSuccess;
}

MStatus uninitializePlugin(MObject object) {
	MStatus status;

	MFnPlugin fnPlugin(object);

	// Distance context ///////////////////////////////////////////////////////////////////////////
	status = fnPlugin.deregisterNode(DistanceSectionManip::id);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	
	status = fnPlugin.deregisterNode(DistanceManipContainer::id);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	status = fnPlugin.deregisterContextCommand(DISTANCE_CONTEXT_NAME, DISTANCE_COMMAND_NAME);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Section node ///////////////////////////////////////////////////////////////////////////////
	status = fnPlugin.deregisterNode(CrossSectionsPlaneManip::id);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	status = fnPlugin.deregisterNode(CrossSectionsManipContainer::id);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	status = MHWRender::MDrawRegistry::deregisterDrawOverrideCreator(
		CrossSectionsNode::drawDbNODE_CLASSIFICATION,
		CrossSectionsNode::drawRegistrantId);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	status = fnPlugin.deregisterNode(CrossSectionsNode::id);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	status = fnPlugin.deregisterCommand(COMMAND_NAME);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	status = MMessage::removeCallbacks(CrossSectionsNode::clipCallbacks);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	return MS::kSuccess;
}