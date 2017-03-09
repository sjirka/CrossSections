#pragma once

#include <maya\MObjectArray.h>
#include <maya\MColorArray.h>
#include <maya\MMatrix.h>
#include <maya\MPointArray.h>
#include <maya\MIntArray.h>
#include <maya\MMatrixArray.h>
#include <map>

class DistanceContext;

struct CrossSectionsData{
	// Numeric
	unsigned int	numberOfPlanes;
	double			planeSpacing;
	float			lineWidth;
	MMatrix			matrixInverse;

	// Curvature
	bool	curvatureComb,
			crvFilter;
	int		crvSamples,
			crvRadius;
	float	crvScale;

	// Input data
	std::map <unsigned int, MObject> inputGeometry;
	std::map <unsigned int, MMatrix> inputMatrix;
	std::map <unsigned int, MColor> inputColor;
	
	// Output data
	std::map <unsigned int, MObjectArray> sectionCurves;
	std::map <unsigned int, std::map<unsigned int, MPointArray>> samplePoints;
	std::map <unsigned int, std::map<unsigned int, MPointArray>> crvPoints;

	// Active context
	DistanceContext *ctxPtr = NULL;
};