#include "../_library/SCamera.h"
#include "CrossSectionsManipContainer.h"
#include "CrossSectionsNode.h"
#include "CrossSectionsData.h"
#include "CrossSectionsPlaneManip.h"
#include "pluginSetup.h"

#include <maya\MFnManip3D.h>

MTypeId CrossSectionsManipContainer::id(MANIP_CONTAINER_ID);

CrossSectionsManipContainer::CrossSectionsManipContainer() {
}

CrossSectionsManipContainer::~CrossSectionsManipContainer() {
}

void * CrossSectionsManipContainer::creator() {
	return new CrossSectionsManipContainer();
}

MStatus CrossSectionsManipContainer::initialize(){
	MStatus status;
	status = MPxManipContainer::initialize();
	return status;
}

MStatus CrossSectionsManipContainer::createChildren(){
	MStatus status;

	// Add plane spacing manipulator /////////////////////////////////////////////////////////////////
	MString manipTypeName = "CrossSectionsManipContainer";
	MString manipName = "distance";
	m_pathDistanceManip = addDistanceManip(manipTypeName, manipName);
	MFnDistanceManip distanceManipFn(m_pathDistanceManip);
	
	// Add toogle toggle axis manips
	std::map <char*, MVector> axis = {
		{"xNegFlip", MVector::xNegAxis},
		{"yFlip", MVector::yAxis},
		{"yNegFlip", MVector::yNegAxis},
		{"zFlip", MVector::zAxis},
		{"zNegFlip", MVector::zNegAxis} };

	for(auto &element : axis)
		m_planePtr.insert( createPlaneManip(element.first, element.second) );

	return MStatus::kSuccess;
}

MStatus CrossSectionsManipContainer::connectToDependNode(const MObject& node){
	MStatus status;

	MFnDependencyNode fnUserNode(node);
	CrossSectionsNode *fnPluginNode = dynamic_cast<CrossSectionsNode*>(fnUserNode.userNode());
	m_data = fnPluginNode->getData();

	// Forward parent node path to manips
	MFnDagNode fnNode(node);
	MDagPath path;
	status = fnNode.getPath(path);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	
	for (auto ptr : m_planePtr)
		ptr->setNode(path);

	// Connect distance manip /////////////////////////////////////////////////////////////////////
	MFnDistanceManip fnDistanceManip(m_pathDistanceManip);
	MPlug pPlaneSpacing = fnUserNode.findPlug(CrossSectionsNode::aPlaneSpacing, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	fnDistanceManip.connectToDistancePlug(pPlaneSpacing);
	status = updateScalingFactor();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	unsigned startPointIndex = fnDistanceManip.startPointIndex();
	addPlugToManipConversionCallback(startPointIndex, (plugToManipConversionCallback)& CrossSectionsManipContainer::startPointCallback);

	unsigned directionIndex = fnDistanceManip.directionIndex();
	addPlugToManipConversionCallback(directionIndex, (plugToManipConversionCallback)& CrossSectionsManipContainer::directionCallback);

	finishAddingManips();
	MPxManipContainer::connectToDependNode(node);

	return MStatus::kSuccess;
}

void CrossSectionsManipContainer::draw(M3dView& view, const MDagPath& path, M3dView::DisplayStyle style, M3dView::DisplayStatus stat){
	MStatus status;

	MPxManipContainer::draw(view, path, style, stat);

	status = updateScalingFactor();
	CHECK_MSTATUS(status);

	status = getManipPoints(m_manipPoints);
	CHECK_MSTATUS(status);

	view.beginGL();

	glPushAttrib(MGL_ALL_ATTRIB_BITS);
	
	// Draw rectangle showing plane orientation
	double planeScale = MANIP_SCALE*SCamera::scaleFactor(view, m_manipPoints[0])/5*MFnManip3D::globalSize();
	status = getRectanglePoints(m_planePoints, getScaledMatrix(m_data->matrixInverse.inverse(), planeScale));
	CHECK_MSTATUS(status);

	glBegin(MGL_LINE_LOOP);
	for (unsigned i = 0; i < 4; i++)
		glVertex3d(m_planePoints[i].x, m_planePoints[i].y, m_planePoints[i].z);
	glEnd();
	
	// Draw a line for distance manip
	glBegin(GL_LINES);
	for (unsigned int i = 0; i < 2; i++) {
		double position[4];
		m_manipPoints[i].get(position);
		glVertex4dv(position);
	}
	glEnd();

	// Draw a point for each extra plane and connect them with a line
	MColor dimmed = view.colorAtIndex(2, M3dView::kDormantColors);
	glColor3f(dimmed.r, dimmed.g, dimmed.b);
	if (2 < m_manipPoints.length()) {
		glBegin(GL_LINE_STRIP);
		for (unsigned int i = 1; i < m_manipPoints.length(); i++)
			glVertex3d(m_manipPoints[i].x, m_manipPoints[i].y, m_manipPoints[i].z);
		glEnd();

		glPointSize(5.0f);
		glBegin(GL_POINTS);
		for (unsigned int i = 2; i < m_manipPoints.length(); i++)
			glVertex3d(m_manipPoints[i].x, m_manipPoints[i].y, m_manipPoints[i].z);
		glEnd();
	}

	glPopAttrib();
	view.endGL();

}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Viewport 2.0 override //////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CrossSectionsManipContainer::preDrawUI(const M3dView &view) {
	MStatus status;

	status = updateScalingFactor();
	CHECK_MSTATUS(status);

	status = getManipPoints(m_manipPoints);
	CHECK_MSTATUS(status);

	double planeScale = MANIP_SCALE*SCamera::scaleFactor(M3dView(view), m_manipPoints[0]) / 5 * MFnManip3D::globalSize();
	status = getRectanglePoints(m_planePoints, getScaledMatrix(m_data->matrixInverse.inverse(), planeScale));
	CHECK_MSTATUS(status);
	m_planePoints.append(m_planePoints[0]);

	M3dView *viewPtr = const_cast<M3dView*>(&view);

	m_activeColor = viewPtr->colorAtIndex(17);
	m_dimmedColor = viewPtr->colorAtIndex(2, M3dView::kDormantColors);
}

void CrossSectionsManipContainer::drawUI(MHWRender::MUIDrawManager &drawManager, const MHWRender::MFrameContext &frameContext) const {
	drawManager.beginDrawable();
	
	drawManager.setColor(m_activeColor);
	drawManager.lineStrip(m_planePoints, false);

	drawManager.line(m_manipPoints[0], m_manipPoints[1]);
	if (2 < m_manipPoints.length()) {
		drawManager.setColor(m_dimmedColor);
		
		MPointArray lines, points;
		for (unsigned int i = 1; i < m_manipPoints.length(); i++)
			lines.append(m_manipPoints[i]);
		for (unsigned int i = 2; i < m_manipPoints.length(); i++)
			points.append(m_manipPoints[i]);
		
		drawManager.setLineStyle(MHWRender::MUIDrawManager::kDotted);
		drawManager.lineStrip(lines, false);
		drawManager.setPointSize(5);
		drawManager.points(points, false);
	}

	drawManager.endDrawable();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions ///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

MManipData CrossSectionsManipContainer::startPointCallback(unsigned index) const{
	MFnNumericData numData;
	MObject numDataObj = numData.create(MFnNumericData::k3Double);
	MPoint start = MPoint::origin*m_data->matrixInverse.inverse();
	numData.setData(start.x, start.y, start.z);
	return MManipData(numDataObj);
}

MManipData CrossSectionsManipContainer::directionCallback(unsigned index) const{
	MFnNumericData numData;
	MObject numDataObj = numData.create(MFnNumericData::k3Double);
	MVector direction = MVector::xAxis*m_data->matrixInverse.inverse();
	numData.setData(direction.x, direction.y, direction.z);
	return MManipData(numDataObj);
}

MStatus CrossSectionsManipContainer::updateScalingFactor() {
	MStatus status;

	MTransformationMatrix tMatrix(m_data->matrixInverse.inverse());
	double scale[3];
	status = tMatrix.getScale(scale, MSpace::kWorld);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	MFnDistanceManip fnDistanceManip(m_pathDistanceManip);
	MDistance conversion;
	status = fnDistanceManip.setScalingFactor(conversion.uiToInternal(scale[1]));
	CHECK_MSTATUS_AND_RETURN_IT(status);

	return MS::kSuccess;
}

MStatus	CrossSectionsManipContainer::getManipPoints(MPointArray &points) {
	MStatus status;
	status = points.clear();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	double spacing = m_data->planeSpacing;
	unsigned int numPoints = (m_data->numberOfPlanes > 2) ? m_data->numberOfPlanes : 2;
	MMatrix transform = m_data->matrixInverse.inverse();
	
	MPoint start(MPoint::origin*transform);
	MVector direction(MVector::xAxis*transform);
	
	MPointArray planePoints;
	for (unsigned int i = 0; i < numPoints; i++) {
		status = points.append(start + direction*spacing*i);
		CHECK_MSTATUS_AND_RETURN_IT(status);
	}

	return MS::kSuccess;
}

MStatus	CrossSectionsManipContainer::getRectanglePoints(MPointArray& points, MMatrix& transform, double offset) {
	MStatus status;

	status = points.clear();
	CHECK_MSTATUS_AND_RETURN_IT(status);

	for (unsigned int i = 0; i < 4; i++) {
		double angle = M_PI / 4 + M_PI / 2 * i;
		MPoint corner = MPoint(offset, sin(angle), cos(angle)) * transform;
		status = points.append(corner);
		CHECK_MSTATUS_AND_RETURN_IT(status);
	}

	return MS::kSuccess;
}

MMatrix	CrossSectionsManipContainer::getScaledMatrix(MMatrix& matrix, double scale){
	MTransformationMatrix tMatrix(matrix);
	double newScale[3] = { scale,scale,scale };
	tMatrix.setScale(newScale, MSpace::kWorld);
	return tMatrix.asMatrix();
}

CrossSectionsPlaneManip* CrossSectionsManipContainer::createPlaneManip(char *name, MVector& axis){
	MPxManipulatorNode *proxyManip = 0;
	MString manipTypeName = PLANE_MANIP_NAME;
	addMPxManipulatorNode(manipTypeName, name, proxyManip);
	CrossSectionsPlaneManip *planePtr = (CrossSectionsPlaneManip*)proxyManip;
	if (planePtr != 0)
		planePtr->setAxis(axis);
	return planePtr;
}