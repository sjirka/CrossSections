#pragma once

#include <maya\MPxToolCommand.h>
#include <maya\MPointArray.h>
#include <maya\MDagModifier.h>

class DistanceToolCmd : public MPxToolCommand
{
public:
	DistanceToolCmd();
	virtual         ~DistanceToolCmd();
	static void*    creator();

	MStatus         doIt(const MArgList& args);
	MStatus         redoIt();
	MStatus         undoIt();
	bool            isUndoable() const;
	MStatus         finalize();

	void setPoints(MPointArray& points);

private:
	MDagModifier m_dagMod;
	MPointArray m_points;
};