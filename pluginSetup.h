#pragma once

// Node
#define NODE_ID					0x0012788A
#define NODE_NAME				"crossSections"
#define NODE_CLASSIFICATION		"drawdb/geometry/crossSections"
#define NODE_REGISTRANT_ID		"CrossSectionsNodePlugin"

#define MANIP_CONTAINER_ID		0x0012788B
#define MANIP_CONTAINER_NAME	"crossSectionsManip"

#define PLANE_MANIP_ID			0x0012788C
#define PLANE_MANIP_NAME		"crossSectionsPlaneManip"
#define MANIP_SCALE				0.25

// Command
#define COMMAND_NAME					"crossSections"

#define COMMAND_NUM_PLANES_FLAG			"-n"
#define COMMAND_NUM_PLANES_FLAG_LONG	"-numberOfPlanes"

#define COMMAND_SPACING_FLAG			"-s"
#define COMMAND_SPACING_FLAG_LONG		"-planeSpacing"

#define COMMAND_TRANSLATE_FLAG			"-t"
#define COMMAND_TRANSLATE_FLAG_LONG		"-translate"

#define COMMAND_ROTATE_FLAG				"-r"
#define COMMAND_ROTATE_FLAG_LONG		"-rotate"

#define COMMAND_ADD_FLAG				"-add"
#define COMMAND_ADD_FLAG_LONG			"-addGeometry"

#define COMMAND_REMOVE_FLAG				"-rem"
#define COMMAND_REMOVE_FLAG_LONG		"-remove"

#define COMMAND_INPUTS_FLAG				"-in"
#define COMMAND_INPUTS_FLAG_LONG		"-inputs"

#define COMMAND_OBJECTS_FLAG			"-obj"
#define COMMAND_OBJECTS_FLAG_LONG		"-objects"

#define COMMAND_CURVES_FLAG				"-crv"
#define COMMAND_CURVES_FLAG_LONG		"-curves"

#define COMMAND_LINEWIDTH_FLAG			"-lw"
#define COMMAND_LINEWIDTH_FLAG_LONG		"-lineWidth"

#define COMMAND_COLOR_FLAG				"-c"
#define COMMAND_COLOR_FLAG_LONG			"-color"

#define COMMAND_COMB_FLAG				"-cc"
#define COMMAND_COMB_FLAG_LONG			"-curvatureComb"

#define COMMAND_SAMPLES_FLAG			"-csp"
#define COMMAND_SAMPLES_FLAG_LONG		"-combSamples"

#define COMMAND_SCALE_FLAG				"-csc"
#define COMMAND_SCALE_FLAG_LONG			"-combScale"

#define COMMAND_SMOOTH_FLAG				"-csm"
#define COMMAND_SMOOTH_FLAG_LONG		"-combSmooth"

#define COMMAND_RADIUS_FLAG				"-csr"
#define COMMAND_RADIUS_FLAG_LONG		"-combSmoothRadius"

#define COMMAND_CLIP_FLAG				"-vc"
#define COMMAND_CLIP_FLAG_LONG			"-visualClip"

// Context
#define DISTANCE_CONTEXT_NAME			"crossSectionsDistance"
#define DISTANCE_COMMAND_NAME			"crossSectionsDistanceCmd"

#define DISTANCE_CONTAINER_ID			0x0012788D
#define DISTANCE_CONTAINER_NAME			"crossSectionsDistanceManip"

#define CURVE_MANIP_ID					0x0012788E
#define CURVE_MANIP_NAME				"crossSectionsCurveManip"

#define GL_CLIP_PLANE6 0x3006