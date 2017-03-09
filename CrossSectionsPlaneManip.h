#pragma once
#include "CrossSectionsData.h"

#include <maya/MPxManipulatorNode.h>
#include <maya/MGlobal.h>
#include <maya/MPointArray.h>
#include <maya/MTransformationMatrix.h>

class CrossSectionsPlaneManip : public MPxManipulatorNode
{
public:
	CrossSectionsPlaneManip();
	virtual ~CrossSectionsPlaneManip();
	virtual void postConstructor() { glFirstHandle(m_manipName); };

	static void* creator();
	static MStatus initialize();

	virtual void draw(M3dView& view, const MDagPath& path, M3dView::DisplayStyle style, M3dView::DisplayStatus status);
	
	virtual void preDrawUI(const M3dView &view);
	virtual void drawUI(MHWRender::MUIDrawManager& drawManager, const MHWRender::MFrameContext& frameContext) const;

	virtual MStatus	doPress(M3dView &veiw) { return MS::kSuccess; };
	virtual MStatus	doDrag(M3dView &view) { return MS::kSuccess; };
	virtual MStatus doRelease(M3dView& view);

	void setAxis(MVector& axis);
	void setNode(MDagPath& path);
	void rotateMatrix(MMatrix& matrix, MMatrix& rotatedMatrix);

	static MTypeId id;

private:
	MDagPath m_path;
	
	MVector m_axis;
	MMatrix m_rotatedMatrix;

	int m_colorIndex;

	MGLuint m_manipName;
	MPointArray m_manipPoints;

};