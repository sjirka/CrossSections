#include "DistanceCmd.h"
#include "DistanceContext.h"

DistanceCmd::DistanceCmd(){
}

DistanceCmd::~DistanceCmd(){
}

void *DistanceCmd::creator() {
	return new DistanceCmd;
}

MPxContext* DistanceCmd::makeObj() {
	return new DistanceContext;
}

MStatus DistanceCmd::appendSyntax() {
	return MS::kSuccess;
}

MStatus DistanceCmd::doEditFlags() {
	return MS::kSuccess;
}
MStatus DistanceCmd::doQueryFlags() {
	return MS::kSuccess;
}