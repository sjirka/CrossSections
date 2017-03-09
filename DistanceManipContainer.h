#pragma once
#include "DistanceSectionManip.h"
#include "CrossSectionsData.h"

#include <maya\MPxManipContainer.h>
#include <maya\MDagPathArray.h>
#include "DistanceContext.h"
#include <set>

class DistanceManipContainer : public MPxManipContainer
{
public:
	DistanceManipContainer();
	virtual ~DistanceManipContainer();

	static void * creator();
	static MStatus initialize();
	virtual MStatus createChildren();
	virtual MStatus connectToDependNode(const MObject& node);

	virtual void 	draw(M3dView &view, const MDagPath &path, M3dView::DisplayStyle style, M3dView::DisplayStatus status);
	virtual void 	preDrawUI(const M3dView &view);
	virtual void 	drawUI(MHWRender::MUIDrawManager &drawManager, const MHWRender::MFrameContext &frameContext) const;

	void			setPoint(MPoint& point, bool append = true);
	MPointArray		getPoints();

	static MTypeId id;

	std::set <DistanceSectionManip*> m_sectionPtr;
private:
	MPointArray m_points;
};