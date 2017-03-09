#include "CrossSectionsDrawOverride.h"
#include "CrossSectionsData.h"
#include "CrossSectionsNode.h"

#include <maya\MFnNurbsCurve.h>

CrossSectionsDrawOverride::CrossSectionsDrawOverride(const MObject& obj) : MHWRender::MPxDrawOverride(obj, CrossSectionsDrawOverride::draw)
{
}

CrossSectionsDrawOverride::~CrossSectionsDrawOverride()
{
}

MHWRender::DrawAPI CrossSectionsDrawOverride::supportedDrawAPIs() const{
	return (MHWRender::kOpenGL | MHWRender::kDirectX11 | MHWRender::kOpenGLCoreProfile);
}


bool CrossSectionsDrawOverride::isBounded(const MDagPath&, const MDagPath&) const{
	return false;
}

MBoundingBox CrossSectionsDrawOverride::boundingBox(const MDagPath& objPath, const MDagPath& cameraPath) const{
	MStatus status;

	MFnDependencyNode node(objPath.node());
	CrossSectionsNode* pluginNode = dynamic_cast<CrossSectionsNode*>(node.userNode());
	CrossSectionsData* nodeData = pluginNode->getData();

	MBoundingBox bBox;
	bBox.transformUsing(nodeData->matrixInverse);
	bBox.expand(MPoint(-1, -1, -1));
	bBox.expand(MPoint(1, 1, 1));

	for (auto const& element : nodeData->sectionCurves) {
		for (unsigned int i = 0; i < element.second.length(); i++) {
			MFnNurbsCurve fnCurve(element.second[i], &status);
			CHECK_MSTATUS(status);
			MPointArray curvePoints;
			status = fnCurve.getCVs(curvePoints);
			CHECK_MSTATUS(status);

			for (unsigned int i = 0; i < curvePoints.length(); i++)
				bBox.expand(curvePoints[i]);
		}
	}

	return bBox;
}

MUserData* CrossSectionsDrawOverride::prepareForDraw(const MDagPath& objPath, const MDagPath& cameraPath, const MHWRender::MFrameContext& frameContext, MUserData* oldData){
	MStatus status;

	MFnDagNode fnNode(objPath);
	MPlug pSection = fnNode.findPlug(CrossSectionsNode::aOutput, &status);
	CHECK_MSTATUS(status);
	MObject oSection;
	status = pSection.getValue(oSection);
	
	return oldData;
}

void CrossSectionsDrawOverride::addUIDrawables(const MDagPath& objPath, MHWRender::MUIDrawManager& drawManager, const MHWRender::MFrameContext& frameContext, const MUserData* data){
	MStatus status; 

	MFnDependencyNode node(objPath.node());
	CrossSectionsNode* pluginNode = dynamic_cast<CrossSectionsNode*>(node.userNode());
	CrossSectionsData* nodeData = pluginNode->getData();

	drawManager.beginDrawable();
	drawManager.setLineWidth(nodeData->lineWidth);
	drawManager.setDepthPriority(7);

	// Draw sections
	for (auto const& element : nodeData->sectionCurves) {
		drawManager.setColor(nodeData->inputColor[element.first]);
		for (unsigned int i = 0; i <element.second.length(); i++) {

			MFnNurbsCurve fnCurve(element.second[i], &status);
			CHECK_MSTATUS(status);
			MPointArray curvePoints;
			status = fnCurve.getCVs(curvePoints);
			CHECK_MSTATUS(status);

			status = drawManager.lineStrip(curvePoints, false);
			CHECK_MSTATUS(status);
		}
	}

	// Draw comb
	if (nodeData->curvatureComb) {
		MColor green(0.0, 0.2f, 0.0);
		drawManager.setColor(green);
		drawManager.setLineWidth(1.0f);

		for (auto const& element : nodeData->crvPoints)
			for (auto const& curve : element.second)
				drawManager.lineStrip(curve.second, false);

		MColor red(1.0f, 0.0, 0.0);
		drawManager.setColor(red);

		for (auto const& element : nodeData->crvPoints)
			for (auto const& curve : element.second)
				for (unsigned int i = 0; i < curve.second.length(); i++)
					drawManager.line(curve.second[i], nodeData->samplePoints[element.first][curve.first][i]);
	}

	drawManager.endDrawable();
}