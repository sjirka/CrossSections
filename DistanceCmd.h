#pragma once

#include <maya\MPxContextCommand.h>

class DistanceCmd : public MPxContextCommand
{
public:
	DistanceCmd();
	virtual ~DistanceCmd();
	static void* creator();

	virtual MStatus doEditFlags();
	virtual MStatus doQueryFlags();

	virtual MPxContext* makeObj();
	virtual MStatus appendSyntax();
private:
};