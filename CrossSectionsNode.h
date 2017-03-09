#pragma once
#include "CrossSectionsData.h"
#include "SSectionPlane.h"
#include <map>

#include <maya\MPxLocatorNode.h>
#include <maya\MMeshSmoothOptions.h>
#include <maya\MTesselationParams.h>
#include <maya\MPointArray.h>
#include <maya\MItMeshPolygon.h>
#include <maya\MItMeshEdge.h>
#include <maya\MItMeshVertex.h>
#include <maya\MDagMessage.h>
#include <maya\MDGModifier.h>
#include <maya\MNodeMessage.h>
#include <maya\MDagPath.h>
#include <maya\MBoundingBox.h>

class CrossSectionsNode : public MPxLocatorNode
{
public:
	CrossSectionsNode();
	virtual ~CrossSectionsNode();
	static void* creator();
	static MStatus	initialize();
	virtual void postConstructor();

	virtual MStatus	setDependentsDirty(const MPlug &plug, MPlugArray &plugArray);
	virtual MStatus	compute(const MPlug &plug, MDataBlock &dataBlock);
	void draw(M3dView &view, const MDagPath &path, M3dView::DisplayStyle style, M3dView::DisplayStatus status);

	virtual bool drawLast()	const { return true; };
	virtual bool isBounded() const;
	virtual MBoundingBox boundingBox() const;

	static void preRender(const MString &panel, void *data);
	static void postRender(const MString &panel, void *data);
	static void parentChanged(MDagPath &child, MDagPath &parent, void *data);
	static void attributeChanged(MNodeMessage::AttributeMessage msg, MPlug & plug, MPlug & otherPlug, void *data);
	static void connectionChanged(MPlug &srcPlug, MPlug &destPlug, bool made, void *clientData);
	static void nodeDeleted(MObject &node, void *data);
	static void meshCreated(MObject &node, void *data);

	CrossSectionsData*	getData();
	void				clearIndex(unsigned int index);

	static MTypeId	id;
	static MString	drawDbNODE_CLASSIFICATION;
	static MString	drawRegistrantId;

	static MObject	aOutput;
	static MObject	aOutputSections;

	static MObject	aNodeMatrix;
	static MObject	aNumberOfPlanes;
	static MObject	aPlaneSpacing;
	static MObject	aLineWidth;
	static MObject  aVisualClip;

	static MObject	aCurvatureComb;
	static MObject	aCombSamples;
	static MObject	aCombScale;
	static MObject	aCombSmooth;
	static MObject	aCombSmoothRadius;

	static MObject	aInput;
	static MObject	aInputMesh;
	static MObject	aInputMatrix;
	static MObject	aInputColor;

	static MObject	activeClipPlane;

	static MCallbackIdArray clipCallbacks;

private:
	// Intersections
	MStatus generateMeshSections(MObject &mesh, MMatrix& matrix, MObjectArray &sectionCurves);

	CrossSectionsData	m_data;

	bool	m_initialize,
			m_dirtyNode,
			m_dirtyLineWidth,
			m_dirtyInput,
			m_dirtyComb;

	MIntArray m_callbackIDs;

	std::map <unsigned int, bool>	m_dirtyGeometry,
									m_dirtySections,
									m_dirtyCurvature,
									m_dirtyMatrix,
									m_dirtyColor;
};

