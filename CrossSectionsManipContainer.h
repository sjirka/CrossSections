#pragma once
#include "CrossSectionsData.h"
#include "CrossSectionsPlaneManip.h"

#include <maya/MPxManipContainer.h>
#include <maya/MFnDistanceManip.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDagPath.h>
#include <maya/MFnNumericData.h>
#include <maya/MManipData.h>
#include <maya/MUIDrawManager.h>
#include <maya/MDistance.h>
#include <maya/MMatrix.h>
#include <maya/MPointArray.h>
#include <set>
#include <map>

class CrossSectionsManipContainer : public MPxManipContainer
{
public:
	CrossSectionsManipContainer();
	virtual ~CrossSectionsManipContainer();

	static void * creator();
	static MStatus initialize();
	virtual MStatus createChildren();
	virtual MStatus connectToDependNode(const MObject& node);

	virtual void draw(M3dView& view, const MDagPath& path, M3dView::DisplayStyle style, M3dView::DisplayStatus status);
	
	virtual void preDrawUI(const M3dView &view);
	virtual void drawUI(MHWRender::MUIDrawManager &drawManager, const MHWRender::MFrameContext &frameContext) const;
	
	static MStatus	getRectanglePoints(MPointArray& points, MMatrix& transform = MMatrix(), double offset=0);
	static MMatrix	getScaledMatrix(MMatrix& matrix, double scale = 1);

	static MTypeId id;

private:
	MManipData	startPointCallback(unsigned index) const;
	MManipData	directionCallback(unsigned index) const;
	MStatus		updateScalingFactor();
	MStatus		getManipPoints(MPointArray& points);
	CrossSectionsPlaneManip* createPlaneManip(char *name, MVector& axis);

	MDagPath	m_pathDistanceManip;

	std::set <CrossSectionsPlaneManip*> m_planePtr;

	MPointArray m_manipPoints,
				m_planePoints;

	MColor	m_activeColor,
			m_dimmedColor;

	CrossSectionsData *m_data = NULL;

};