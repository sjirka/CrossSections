#include "pluginSetup.h"
#include "DistanceToolCmd.h"

#include <maya\MArgList.h>
#include <maya\MFnDagNode.h>
#include <maya\MPlugArray.h>
#include <maya\MPlug.h>
#include <maya\MDistance.h>

DistanceToolCmd::DistanceToolCmd() {
	setCommandString(DISTANCE_COMMAND_NAME);
}

DistanceToolCmd::~DistanceToolCmd() {
}

void* DistanceToolCmd::creator() {
	return new DistanceToolCmd;
}

bool DistanceToolCmd::isUndoable() const {
	return true;
}

MStatus DistanceToolCmd::finalize() {
	MStatus status;
	MArgList command;
	status = command.addArg(commandString());
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = MPxToolCommand::doFinalize(command);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	return MS::kSuccess;
}

MStatus DistanceToolCmd::doIt(const MArgList& args) {
	return redoIt();
}

MStatus DistanceToolCmd::undoIt() {
	MStatus status;

	status = m_dagMod.undoIt();
	CHECK_MSTATUS_AND_RETURN_IT(status)

	return MStatus::kSuccess;
}

MStatus DistanceToolCmd::redoIt() {
	MStatus status;

	if (m_points.length() == 2) {
		MObject oTrans = m_dagMod.createNode("transform", MObject::kNullObj, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		MFnDagNode fnTrans(oTrans);
		fnTrans.setName("sectionDistance");

		MObject oDimension = m_dagMod.createNode("distanceDimShape", oTrans, &status);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		MFnDagNode fnDimension(oDimension);
		fnDimension.setName("sectionDistanceShape");

		MPlugArray pPoints;
		pPoints.append(fnDimension.findPlug("startPoint", &status));
		CHECK_MSTATUS_AND_RETURN_IT(status);
		pPoints.append(fnDimension.findPlug("endPoint", &status));
		CHECK_MSTATUS_AND_RETURN_IT(status);

		for (unsigned int i = 0; i < pPoints.length(); i++) {
			double position[4];
			m_points[i].get(position);
			for (unsigned int c = 0; c < pPoints[i].numChildren(); c++) {
				m_dagMod.newPlugValueMDistance(pPoints[i].child(c), MDistance::internalToUI(position[c]));
			}
		}

	status = m_dagMod.doIt();
	CHECK_MSTATUS_AND_RETURN_IT(status);
	}

	return MStatus::kSuccess;
}

void DistanceToolCmd::setPoints(MPointArray& points) {
	m_points = points;
}