//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================


//  06/25/2002 MAH
//  This header file has been modified to now include the proper BSP model
//  structure definitions for each of the two Quakeworld client renderers:
//  software mode and GL mode. Originally, Valve only supplied it with
//  the software mode definitions, which caused General Protection Fault's
//  when accessing members of the structures that are different between
//  the two versions.  These are: 'mnode_t', 'mleaf_t', 'msurface_t' and
//  'texture_t'. To select the GL hardware rendering versions of these
//  structures, define 'HARDWARE_MODE' as a preprocessor symbol, otherwise
//  it will default to software mode as supplied.

// Hardcoded HARDWARE_MODE - Max Vollmer, 2018-01-21
#ifndef HARDWARE_MODE
#define HARDWARE_MODE
#endif


// com_model.h
#ifndef COM_MODEL_H
#define COM_MODEL_H

#define STUDIO_RENDER 1
#define STUDIO_EVENTS 2

#define MAX_CLIENTS 32
#define MAX_EDICTS  3072

#define SURF_PLANEBACK 2  // plane should be negated // Added from bspfile.h - Max Vollmer, 2018-02-04

#define MAX_MODEL_NAME 64
#define MAX_MAP_HULLS  4
#define MIPLEVELS      4
#define NUM_AMBIENTS   4  // automatic ambient sounds
#define MAXLIGHTMAPS   4
#define PLANE_ANYZ     5

#define ALIAS_Z_CLIP_PLANE 5

// flags in finalvert_t.flags
#define ALIAS_LEFT_CLIP    0x0001
#define ALIAS_TOP_CLIP     0x0002
#define ALIAS_RIGHT_CLIP   0x0004
#define ALIAS_BOTTOM_CLIP  0x0008
#define ALIAS_Z_CLIP       0x0010
#define ALIAS_ONSEAM       0x0020
#define ALIAS_XY_CLIP_MASK 0x000F

#define ZISCALE ((float)0x8000)

#define CACHE_SIZE 32  // used to align key data structures

typedef enum
{
	mod_brush,
	mod_sprite,
	mod_alias,
	mod_studio
} modtype_t;

// must match definition in modelgen.h
#ifndef SYNCTYPE_T
	#define SYNCTYPE_T

typedef enum
{
	ST_SYNC = 0,
	ST_RAND
} synctype_t;

#endif

typedef struct
{
	float mins[3] = { 0 }, maxs[3] = { 0 };
	float origin[3] = { 0 };
	int headnode[MAX_MAP_HULLS] = { 0 };
	int visleafs = 0;  // not including the solid leaf 0
	int firstface = 0, numfaces = 0;
} dmodel_t;

// plane_t structure
// 06/23/2002 MAH
// This structure is the same in QW source files
//  'model.h' and 'gl_model.h'
typedef struct mplane_s
{
	vec3_t normal;  // surface normal
	float dist = 0.f;     // closest appoach to origin
	byte type = 0;      // for texture axis selection and fast side tests
	byte signbits = 0;  // signx + signy<<1 + signz<<1
	byte pad[2] = { 0 };
} mplane_t;

typedef struct
{
	vec3_t position;
} mvertex_t;

// 06/23/2002 MAH
// This structure is the same in QW source files
//  'model.h' and 'gl_model.h'
typedef struct
{
	unsigned short v[2] = { 0 };
	unsigned int cachededgeoffset = 0;
} medge_t;


//
//  hardware mode - QW 'gl_model.h'
typedef struct texture_s
{
	char name[16] = { 0 };
	unsigned width = 0, height = 0;
	int gl_texturenum = 0;
	struct msurface_s* texturechain = nullptr;    // for gl_texsort drawing
	int anim_total = 0;                     // total tenths in sequence ( 0 = no)
	int anim_min = 0, anim_max = 0;             // time for this frame min <=time< max
	struct texture_s* anim_next = nullptr;        // in the animation sequence
	struct texture_s* alternate_anims = nullptr;  // bmodels in frmae 1 use these
	unsigned offsets[MIPLEVELS] = { 0 };        // four mip maps stored
} texture_t;


// 06/23/2002 MAH
// This structure is the same in QW source files
//  'model.h' and 'gl_model.h'
typedef struct
{
	float vecs[2][4] = { 0 };  // [s/t] unit vectors in world space.
	                   // [i][3] is the s/t offset relative to the origin.
	                   // s or t = dot(3Dpoint,vecs[i])+vecs[i][3]
	float mipadjust = 0.f;   // ?? mipmap limits for very small surfaces
	texture_t* texture = nullptr;
	int flags = 0;  // sky or slime, no lightmap or 256 subdivision
} mtexinfo_t;

// 06/23/2002 MAH
// This structure is only need for hardware rendering
#define VERTEXSIZE 7

typedef struct glpoly_s
{
	struct glpoly_s* next = nullptr;
	struct glpoly_s* chain = nullptr;
	int numverts = 0;
	int flags = 0;  // for SURF_UNDERWATER

	// 07/24/2003 R9 - not actually four! AHA! AHAHAHAHAHAHA! hack.
	float verts[4][VERTEXSIZE] = { 0 };  // variable sized (xyz s1t1 s2t2)
} glpoly_t;


//
//  hardware mode - QW 'gl_model.h'
typedef struct mnode_s
{
	// common with leaf
	int contents = 0;  // 0, to differentiate from leafs
	int visframe = 0;  // node needs to be traversed if current

	float minmaxs[6] = { 0 };  // for bounding box culling

	struct mnode_s* parent = nullptr;

	// node specific
	mplane_t* plane = nullptr;
	struct mnode_s* children[2] = { 0 };

	unsigned short firstsurface = 0;
	unsigned short numsurfaces = 0;
} mnode_t;


typedef struct msurface_s msurface_t;
typedef struct decal_s decal_t;

// JAY: Compress this as much as possible
struct decal_s
{
	decal_t* pnext = nullptr;        // linked list for each surface
	msurface_t* psurface = nullptr;  // Surface id for persistence / unlinking
	short dx = 0;              // Offsets into surface texture (in texture coordinates, so we don't need floats)
	short dy = 0;
	short texture = 0;  // Decal texture
	byte scale = 0;     // Pixel scale
	byte flags = 0;     // Decal flags

	short entityIndex = 0;  // Entity this is attached to
};

//
//  hardware renderer - QW 'gl_model.h'
typedef struct mleaf_s
{
	// common with node
	int contents = 0;  // wil be a negative contents number
	int visframe = 0;  // node needs to be traversed if current

	float minmaxs[6] = { 0 };  // for bounding box culling

	struct mnode_s* parent = nullptr;

	// leaf specific
	byte* compressed_vis = nullptr;
	struct efrag_s* efrags = nullptr;

	msurface_t** firstmarksurface = nullptr;
	int nummarksurfaces = 0;
	int key = 0;  // BSP sequence number for leaf's contents
	byte ambient_sound_level[NUM_AMBIENTS] = { 0 };
} mleaf_t;

//
//  hardware renderer - QW 'gl_model.h'
//  06/23/2002 2230 MAH
//  WARNING - the above indicates this structure was modified
//      for Half-Life this structure needs VERIFICATION
//  06/23/2002 2300 MAH - the below version for hardware agrees
//      with a hexadecimal data dump of these structures taken
//      from a running game.
typedef struct msurface_s
{
	int visframe = 0;  // should be drawn when node is crossed

	mplane_t* plane = nullptr;
	int flags = 0;

	int firstedge = 0;  // look up in model->surfedges[], negative numbers
	int numedges = 0;   // are backwards edges

	short texturemins[2] = { 0 };
	short extents[2] = { 0 };

	int light_s = 0, light_t = 0;  // gl lightmap coordinates

	glpoly_t* polys = nullptr;  // multiple if warped
	struct msurface_s* texturechain = nullptr;

	mtexinfo_t* texinfo = nullptr;

	// lighting info
	int dlightframe = 0;
	int dlightbits = 0;

	int lightmaptexturenum = 0;
	byte styles[MAXLIGHTMAPS] = { 0 };
	int cached_light[MAXLIGHTMAPS] = { 0 };  // values currently used in lightmap
	qboolean cached_dlight = 0;          // true if dynamic light in cache

	//  byte        *samples;                   // [numstyles*surfsize]
	color24* samples = nullptr;  // note: this is the actual lightmap data for this surface
	decal_t* pdecals = nullptr;

} msurface_t;

//
//  06/23/2002 MAH
//  Note: this structure is exactly the same in QW software
//      and hardware renderers QW - 'bspfile.h'
typedef struct
{
	int planenum = 0;
	short children[2] = { 0 };  // negative numbers are contents
} dclipnode_t;

//
//  06/23/2002 MAH
//  Note: this structure is exactly the same in QW software
//      and hardware renderers 'model.h' and 'gl_model.h'
typedef struct hull_s
{
	dclipnode_t* clipnodes = nullptr;
	mplane_t* planes = nullptr;
	int firstclipnode = 0;
	int lastclipnode = 0;
	vec3_t clip_mins;
	vec3_t clip_maxs;
} hull_t;

#if !defined(CACHE_USER) && !defined(QUAKEDEF_H)
	#define CACHE_USER
typedef struct cache_user_s
{
	void* data = nullptr;
} cache_user_t;
#endif

typedef struct model_s
{
	char name[MAX_MODEL_NAME] = { 0 };  // +0x000
	qboolean needload = 0;          // +0x040   bmodels and sprites don't cache normally

	modtype_t type;       // +0x044
	int numframes = 0;        // +0x048
	synctype_t synctype;  // +0x04C

	int flags = 0;  // +0x050

	//
	// volume occupied by the model
	//
	vec3_t mins, maxs;  // +0x054, +060
	float radius = 0.f;       // +0x06C

	//
	// brush model
	//
	int firstmodelsurface = 0, nummodelsurfaces = 0;  // +0x070, +0x074

	int numsubmodels = 0;     // +0x078
	dmodel_t* submodels = nullptr;  // +0x07C

	int numplanes = 0;     // +0x080
	mplane_t* planes = nullptr;  // +0x084

	int numleafs = 0;           // +0x088      number of visible leafs, not counting 0
	struct mleaf_s* leafs = nullptr;  // +0x08C

	int numvertexes = 0;      // +0x090
	mvertex_t* vertexes = nullptr;  // +0x094

	int numedges = 0;    // +0x098
	medge_t* edges = nullptr;  // +0x09C

	int numnodes = 0;    // +0x0A0
	mnode_t* nodes = nullptr;  // +0x0A4

	int numtexinfo = 0;       // +0x0A8
	mtexinfo_t* texinfo = nullptr;  // +0x0AC

	int numsurfaces = 0;       // +0x0B0
	msurface_t* surfaces = nullptr;  // +0x0B4

	int numsurfedges = 0;
	int* surfedges = nullptr;

	int numclipnodes = 0;
	dclipnode_t* clipnodes = nullptr;

	int nummarksurfaces = 0;
	msurface_t** marksurfaces = nullptr;

	hull_t hulls[MAX_MAP_HULLS] = { 0 };

	int numtextures = 0;
	texture_t** textures = nullptr;

	byte* visdata = nullptr;

	color24* lightdata = nullptr;

	char* entities = nullptr;

	//
	// additional model data
	//
	cache_user_t cache;  // only access through Mod_Extradata

} model_t;

typedef vec_t vec4_t[4];

typedef struct alight_s
{
	int ambientlight = 0;  // clip at 128
	int shadelight = 0;    // clip at 192 - ambientlight
	vec3_t color;
	float* plightvec = nullptr;
} alight_t;

typedef struct auxvert_s
{
	float fv[3] = { 0 };  // viewspace x, y
} auxvert_t;

//
// ------------------  Player Model Animation Info ----------------
//
#include "custom.h"

#define MAX_INFO_STRING    256
#define MAX_SCOREBOARDNAME 32
typedef struct player_info_s
{
	// User id on server
	int userid = 0;

	// User info string
	char userinfo[MAX_INFO_STRING] = { 0 };

	// Name
	char name[MAX_SCOREBOARDNAME] = { 0 };

	// Spectator or not, unused
	int spectator = 0;

	int ping = 0;
	int packet_loss = 0;

	// skin information
	char model[MAX_QPATH] = { 0 };
	int topcolor = 0;
	int bottomcolor = 0;

	// last frame rendered
	int renderframe = 0;

	// Gait frame estimation
	int gaitsequence = 0;
	float gaitframe = 0.f;
	float gaityaw = 0.f;
	vec3_t prevgaitorigin;

	customization_t customdata;
} player_info_t;

#endif  // #define COM_MODEL_H
