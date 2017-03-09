#include "pluginSetup.h"
#include "DistanceSectionManip.h"
#include "DistanceManipContainer.h"
#include "../_library/SPlane.h"

#include <maya\MGlobal.h>
#include <maya\MFnDagNode.h>
#include <maya\M3dView.h>
#include <maya\MFnNurbsCurve.h>

MTypeId DistanceSectionManip::id(CURVE_MANIP_ID);

DistanceSectionManip::DistanceSectionManip() {}

DistanceSectionManip::~DistanceSectionManip() {}

void* DistanceSectionManip::creator() {
	return new DistanceSectionManip;
}

MStatus DistanceSectionManip::initialize() {
	return MStatus::kSuccess;
}

void DistanceSectionManip::postConstructor() {
	MStatus status;

	glFirstHandle(m_id);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Draw ///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DistanceSectionManip::draw(M3dView& view, const MDagPath& path, M3dView::DisplayStyle style, M3dView::DisplayStatus stat) {
	MStatus status;

	if (m_curve.isNull())
		return;

	MFnNurbsCurve fnCurve(m_curve, &status);
	CHECK_MSTATUS(status);
	MPointArray curvePoints;
	status = fnCurve.getCVs(curvePoints);
	CHECK_MSTATUS(status);

	view.beginGL();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glLineWidth(m_lineWidth);

	colorAndName(view, m_id, true, mainColor());
	
	glColor3f(m_color.r, m_color.g, m_color.b);

	glBegin(GL_LINE_STRIP);
	for (unsigned int i = 0; i < curvePoints.length(); i++) {
		curvePoints[i] *= m_matrix.inverse();
		glVertex3d(curvePoints[i].x, curvePoints[i].y, curvePoints[i].z);
	}
	glEnd();

	glPopAttrib();
	view.endGL();
}

void DistanceSectionManip::preDrawUI(const M3dView &view) {
	MStatus status;
}

void DistanceSectionManip::drawUI(MHWRender::MUIDrawManager &drawManager, const MHWRender::MFrameContext &frameContext) const {
	MStatus status;

	if (m_curve.isNull())
		return;

	drawManager.beginDrawable(m_id, true);
	drawManager.setDepthPriority(7);
	drawManager.setColor(m_color);
	drawManager.setLineWidth(m_lineWidth);

	MFnNurbsCurve fnCurve(m_curve, &status);
	CHECK_MSTATUS(status);
	MPointArray curvePoints;
	status = fnCurve.getCVs(curvePoints);
	CHECK_MSTATUS(status);

	for (unsigned int i = 0; i < curvePoints.length(); i++)
		curvePoints[i] *= m_matrix.inverse();

	status = drawManager.lineStrip(curvePoints, false);
	CHECK_MSTATUS(status);

	drawManager.endDrawable();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Event functions ////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

MStatus	DistanceSectionManip::doPress(M3dView &view) {
	MStatus status;

	if (m_curve.isNull())
		return MS::kUnknownParameter;

	MPoint point;
	status = pointOnSection(view, m_curve, point);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	if(NULL != m_manipPtr)
		m_manipPtr->setPoint(point);

	return MStatus::kUnknownParameter;
}

MStatus	DistanceSectionManip::doDrag(M3dView &view) {
	MStatus status;

	if (m_curve.isNull())
		return MS::kUnknownParameter;

	MPoint point;
	status = pointOnSection(view, m_curve, point);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	if (NULL != m_manipPtr)
		m_manipPtr->setPoint(point, false);

	return MStatus::kUnknownParameter;
}

MStatus DistanceSectionManip::doRelease(M3dView &view) {
	return MStatus::kUnknownParameter;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Data functions /////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void DistanceSectionManip::setParentPtr(DistanceManipContainer* manipPtr) {
	m_manipPtr = manipPtr;
}

void DistanceSectionManip::setSectionData(MObject& curve, MColor& color, float lineWidth, MMatrix& matrix) {
	m_curve = curve;
	m_color = color;
	m_lineWidth = lineWidth;
	m_matrix = matrix;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions ///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

MStatus DistanceSectionManip::pointOnSection(M3dView &veiw, const MObject& sectionCurve, MPoint& point) {
	MStatus status;

	MPoint crvPt, rayPt, plPt;
	MVector rayDirection;
	double param;
	
	MFnNurbsCurve fnCurve(sectionCurve, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	status = fnCurve.getCV(0, crvPt);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	SPlane plane(crvPt, MVector::xAxis);
	status = mouseRayWorld(rayPt, rayDirection);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	plane.intersect(rayPt*m_matrix, rayDirection*m_matrix, plPt, param);

	point = fnCurve.closestPoint(plPt, &param, 0.001, MSpace::kObject, &status)*m_matrix.inverse();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	return MS::kSuccess;
}