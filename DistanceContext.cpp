#include "DistanceContext.h"
#include "DistanceManipContainer.h"
#include "DistanceToolCmd.h"
#include "pluginSetup.h"
#include "CrossSectionsNode.h"

#include <maya\MGlobal.h>
#include <maya\MFnDependencyNode.h>
#include <maya\MItDependencyNodes.h>

DistanceContext::DistanceContext(){
	setTitleString("Cross Sections Distance");
	setImage("crossSections.xpm", MPxContext::kImage1);
	
	setAllPointers(this);
}

DistanceContext::~DistanceContext(){
	setAllPointers(NULL);
}

void DistanceContext::toolOnSetup(MEvent &event) {
	setAllPointers(this);
	updateManip();

	setHelpString("Click and drag to select points. Press Enter to create distance locator");
}

void DistanceContext::toolOffCleanup() {
	if (NULL != m_manipPtr)
		clearManip();

	setAllPointers(NULL);

	MPxContext::toolOffCleanup();
}

void DistanceContext::completeAction() {
	if (2 == m_manipPtr->getPoints().length()){
		DistanceToolCmd *toolCmd = (DistanceToolCmd*)newToolCommand();
		toolCmd->setPoints(m_manipPtr->getPoints());
		toolCmd->redoIt();
		toolCmd->finalize();
	}

	updateManip();
}

void DistanceContext::abortAction() {
	updateManip();
}


void DistanceContext::deleteAction() {
	updateManip();
}

void DistanceContext::updateManip() {
	if (NULL != m_manipPtr)
		clearManip();

	MString manipName = DISTANCE_CONTAINER_NAME;
	MObject manipObject;
	m_manipPtr = (DistanceManipContainer *)DistanceManipContainer::newManipulator(manipName, manipObject);
	if (NULL != m_manipPtr)
		addManipulator(manipObject);
}

void DistanceContext::clearManip() {
	for (auto ptr : m_manipPtr->m_sectionPtr)
		ptr->setParentPtr(NULL);
	m_manipPtr = NULL;

	deleteManipulators();
}

MStatus DistanceContext::setAllPointers(DistanceContext *ctxPtr) {
	MStatus status;
	MItDependencyNodes itNodes(MFn::kPluginLocatorNode, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	for (itNodes.reset(); !itNodes.isDone(); itNodes.next()) {
		MFnDependencyNode fnNode(itNodes.thisNode(), &status);
		CHECK_MSTATUS_AND_RETURN_IT(status)

			if (fnNode.typeId() != CrossSectionsNode::id)
				continue;

		CrossSectionsNode *nodePtr = dynamic_cast<CrossSectionsNode*>(fnNode.userNode());
		CrossSectionsData *nodeData = nodePtr->getData();
		nodeData->ctxPtr = ctxPtr;
	}

	return MS::kSuccess;
}