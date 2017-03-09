#include "pluginSetup.h"
#include "CrossSectionsManipContainer.h"
#include "CrossSectionsNode.h"
#include "../_library/SCamera.h"

#include <maya\MFnManip3D.h>
#include <maya\MFnDependencyNode.h>
#include <maya\MQuaternion.h>

MTypeId CrossSectionsPlaneManip::id(PLANE_MANIP_ID);

CrossSectionsPlaneManip::CrossSectionsPlaneManip() {}

CrossSectionsPlaneManip::~CrossSectionsPlaneManip() {}

void* CrossSectionsPlaneManip::creator() {
	return new CrossSectionsPlaneManip();
}

MStatus CrossSectionsPlaneManip::initialize() {
	return MStatus::kSuccess;
}

void CrossSectionsPlaneManip::draw(M3dView& view, const MDagPath& path, M3dView::DisplayStyle style, M3dView::DisplayStatus status) {
	
	MFnDependencyNode fnUserNode(m_path.node());
	CrossSectionsNode *fnPluginNode = dynamic_cast<CrossSectionsNode*>(fnUserNode.userNode());
	CrossSectionsData *nodeData = fnPluginNode->getData();
	
	rotateMatrix(nodeData->matrixInverse.inverse(), m_rotatedMatrix);

	double rectangleScale = MANIP_SCALE*SCamera::scaleFactor(view, MPoint::origin*m_rotatedMatrix) / 15 * MFnManip3D::globalSize();
	MMatrix scaledMatrix = CrossSectionsManipContainer::getScaledMatrix(m_rotatedMatrix, rectangleScale);
	
	m_manipPoints.clear();
	if (MVector::xNegAxis == m_axis){
		m_manipPoints.append(MPoint(0, 0, 0)*scaledMatrix);
		m_manipPoints.append(MPoint(2, 0, 0)*scaledMatrix);
	}
	else{
		CrossSectionsManipContainer::getRectanglePoints(m_manipPoints, scaledMatrix, 2.7);
		m_manipPoints.append(m_manipPoints[0]);
	}
	
	view.beginGL();
	glPushAttrib(MGL_ALL_ATTRIB_BITS);

	glLineStipple(1, 0xAAAA);
	glEnable(MGL_LINE_STIPPLE);

	bool drawAsSelected;
	shouldDrawHandleAsSelected(m_manipName, drawAsSelected);

	colorAndName(view, m_manipName, true, (drawAsSelected) ? selectedColor() : dimmedColor());
	glBegin(GL_LINE_STRIP);
	for (unsigned int i = 0; i < m_manipPoints.length(); i++)
		glVertex3d(m_manipPoints[i].x, m_manipPoints[i].y, m_manipPoints[i].z);
	glEnd();

	glPopAttrib();
	view.endGL();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Viewport 2.0 override //////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////


void CrossSectionsPlaneManip::preDrawUI(const M3dView &view) {

	MFnDependencyNode fnUserNode(m_path.node());
	CrossSectionsNode *fnPluginNode = dynamic_cast<CrossSectionsNode*>(fnUserNode.userNode());
	CrossSectionsData *nodeData = fnPluginNode->getData();

	rotateMatrix(nodeData->matrixInverse.inverse(), m_rotatedMatrix);

	double rectangleScale = MANIP_SCALE*SCamera::scaleFactor(M3dView(view), MPoint::origin*m_rotatedMatrix) / 15 * MFnManip3D::globalSize();
	MMatrix scaledMatrix = CrossSectionsManipContainer::getScaledMatrix(m_rotatedMatrix, rectangleScale);

	m_manipPoints.clear();
	if (MVector::xNegAxis == m_axis) {
		m_manipPoints.append(MPoint(0, 0, 0)*scaledMatrix);
		m_manipPoints.append(MPoint(2, 0, 0)*scaledMatrix);
	}
	else{
		CrossSectionsManipContainer::getRectanglePoints(m_manipPoints, scaledMatrix, 2.7);
		m_manipPoints.append(m_manipPoints[0]);
	}

	bool drawAsSelected;
	shouldDrawHandleAsSelected(m_manipName, drawAsSelected);
	m_colorIndex = (drawAsSelected) ? selectedColor() : dimmedColor();
}

void CrossSectionsPlaneManip::drawUI(MHWRender::MUIDrawManager &drawManager, const MHWRender::MFrameContext &frameContext) const {
	drawManager.beginDrawable(m_manipName, true);
	drawManager.setColorIndex(m_colorIndex);
	drawManager.setLineStyle(MHWRender::MUIDrawManager::kDotted);
	drawManager.lineStrip(m_manipPoints, false);
	drawManager.endDrawable();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Event functions ////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

MStatus CrossSectionsPlaneManip::doRelease(M3dView& view) {
	MStatus status;

	MFnDagNode fnNode(m_path);
	MFnTransform fnTransform(m_path.transform());
	status = fnTransform.set(MTransformationMatrix(m_rotatedMatrix));
	CHECK_MSTATUS_AND_RETURN_IT(status);

	MPlug dummyPlug = fnNode.findPlug(CrossSectionsNode::aOutput, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	dummyPlug.asMObject();

	return MStatus::kSuccess;
}

void CrossSectionsPlaneManip::setAxis(MVector& axis) {
	m_axis = axis;
}

void CrossSectionsPlaneManip::setNode(MDagPath& path) {
	m_path = path;
}

void CrossSectionsPlaneManip::rotateMatrix(MMatrix& matrix, MMatrix& rotatedMatrix) {
	MTransformationMatrix tMatrix(matrix);
	MQuaternion rotation = MVector::xAxis.rotateTo(m_axis);
	tMatrix.addRotationQuaternion(rotation.x, rotation.y, rotation.z, rotation.w, MSpace::kObject);
	rotatedMatrix = tMatrix.asMatrix();
}