#include "DistanceManipContainer.h"
#include "CrossSectionsNode.h"
#include "pluginSetup.h"

#include <maya\MFnDependencyNode.h>
#include <maya\MItDependencyNodes.h>
#include <maya\MDistance.h>

MTypeId DistanceManipContainer::id(DISTANCE_CONTAINER_ID);

DistanceManipContainer::DistanceManipContainer() {
}

DistanceManipContainer::~DistanceManipContainer() {
}

void * DistanceManipContainer::creator() {
	return new DistanceManipContainer;
}

MStatus DistanceManipContainer::initialize(){
	MPxManipContainer::initialize();
	return MS::kSuccess;
}

MStatus DistanceManipContainer::createChildren(){
	MStatus status;

	MPxManipulatorNode *proxyManip = 0;
	MString manipTypeName = CURVE_MANIP_NAME;
	MString manipName = "sections";

	MItDependencyNodes itNodes(MFn::kPluginLocatorNode, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	for (itNodes.reset(); !itNodes.isDone(); itNodes.next()) {
		MFnDependencyNode fnNode(itNodes.thisNode(), &status);
		CHECK_MSTATUS_AND_RETURN_IT(status)

		if (fnNode.typeId() != CrossSectionsNode::id)
			continue;

		CrossSectionsNode *nodePtr = dynamic_cast<CrossSectionsNode*>(fnNode.userNode());
		CrossSectionsData *nodeData = nodePtr->getData();

		for (auto &element : nodeData->sectionCurves) {
			for (unsigned int i = 0; i < element.second.length(); i++){
				proxyManip = NULL;
				status = addMPxManipulatorNode(manipTypeName, fnNode.name() + manipName, proxyManip);
				CHECK_MSTATUS_AND_RETURN_IT(status);
				DistanceSectionManip *manipPtr = (DistanceSectionManip*)proxyManip;
				if (NULL != manipPtr) {
					manipPtr->setParentPtr(this);
					manipPtr->setSectionData(nodeData->sectionCurves[element.first][i], nodeData->inputColor[element.first], nodeData->lineWidth, nodeData->matrixInverse);
					m_sectionPtr.insert(manipPtr);
				}
			}
		}
	}

	return MStatus::kSuccess;
}

MStatus DistanceManipContainer::connectToDependNode(const MObject& node){
	finishAddingManips();
	return MStatus::kSuccess;
}

void DistanceManipContainer::draw(M3dView &view, const MDagPath &path, M3dView::DisplayStyle style, M3dView::DisplayStatus status) {
	MPxManipContainer::draw(view, path, style, status);

	view.beginGL();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glColor3f(1.0f, 1.0f, 0.0f);
	glPointSize(7.0);

	// Draw points
	if (0 < m_points.length()) {
		glBegin(GL_POINTS);
		for (unsigned int i = 0; i < m_points.length(); i++)
			glVertex3d(m_points[i].x, m_points[i].y, m_points[i].z);
		glEnd();
	}

	if (m_points.length() == 2) {
		
		// Draw stripple line
		glLineStipple(1, 0xAAAA);
		glEnable(GL_LINE_STIPPLE);
		glBegin(GL_LINES);
		for (unsigned int i = 0; i < m_points.length(); i++)
			glVertex3d(m_points[i].x, m_points[i].y, m_points[i].z);
		glEnd();

		// Draw distance label
		MVector direction = m_points[1] - m_points[0];
		MPoint center = m_points[0] + direction / 2;
		double distance = MDistance::internalToUI(direction.length());
		char buffer[512];
		sprintf_s(buffer, "%8.2f", distance);

		view.drawText(buffer, center, M3dView::kCenter);
	}

	glPopAttrib();
	view.endGL();
}

void DistanceManipContainer::preDrawUI(const M3dView &view){
}

void DistanceManipContainer::drawUI(MHWRender::MUIDrawManager &drawManager, const MHWRender::MFrameContext &frameContext) const {
	drawManager.beginDrawable();
	drawManager.setDepthPriority(7);

	drawManager.setColor(MColor(1.0, 1.0, 0));
	drawManager.setPointSize(7.0);
	
	// Draw points
	if(0<m_points.length())
		drawManager.points(m_points, false);

	if (m_points.length() == 2) {
		
		// Draw stripple line
		drawManager.setLineStyle(MHWRender::MUIDrawManager::kDotted);
		drawManager.lineList(m_points, false);

		//Draw distance lable
		MVector direction = m_points[1] - m_points[0];
		MPoint center = m_points[0] + direction / 2;
		double distance = MDistance::internalToUI(direction.length());
		char buffer[512];
		sprintf_s(buffer, "%8.2f", distance);

		drawManager.text(center, buffer, MHWRender::MUIDrawManager::kCenter);
	}

	drawManager.endDrawable();
}

void DistanceManipContainer::setPoint(MPoint& point, bool append) {
	if (append) {
		if (2 <= m_points.length())
			m_points.clear();
		m_points.append(point);
	}
	else
		m_points.set(point, m_points.length() - 1);
}

MPointArray DistanceManipContainer::getPoints() {
	return m_points;
}