#pragma once

#include <maya/MPxManipulatorNode.h>
#include <map>

class DistanceManipContainer;

class DistanceSectionManip : public MPxManipulatorNode
{
public:
	DistanceSectionManip();
	virtual ~DistanceSectionManip();
	virtual void postConstructor();

	static void* creator();
	static MStatus initialize();

	virtual void draw(M3dView& view, const MDagPath& path, M3dView::DisplayStyle style, M3dView::DisplayStatus status);
	
	virtual void preDrawUI(const M3dView &view);
	virtual void drawUI(MHWRender::MUIDrawManager& drawManager, const MHWRender::MFrameContext& frameContext) const;

	virtual MStatus	doPress(M3dView &view);
	virtual MStatus	doDrag(M3dView &view);
	virtual MStatus doRelease(M3dView &view);

	void setParentPtr(DistanceManipContainer* manipPtr);
	void setSectionData(MObject& curve, MColor& color, float lineWidth, MMatrix& matrix);

	static MTypeId id;

private:
	MGLuint m_id;
	MObject m_curve;
	MColor m_color;
	float m_lineWidth;
	MMatrix m_matrix;

	DistanceManipContainer *m_manipPtr = NULL;

	MStatus pointOnSection(M3dView &veiw, const MObject& sectionCurve, MPoint& point);
};