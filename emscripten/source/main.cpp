
#include <stdio.h>
#include <math.h>
#include <memory.h>

#include <Recast.h>
#include <InputGeom.h>
#include <SampleInterfaces.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include <DetourNavMeshBuilder.h>
#include <DetourCrowd.h>

#include <emscripten.h>

static const int MAX_POLYS = 256;

////////////////////////////
// CUSTOM
//////////////
rcMeshLoaderObj* meshLoader;

////////////////////////////
// FROM SAMPLE
//////////////
InputGeom* m_geom;
dtNavMesh* m_navMesh;
dtNavMeshQuery* m_navQuery;
dtCrowd* m_crowd;

unsigned char m_navMeshDrawFlags;

float m_cellSize = 0.4f;
float m_cellHeight = 0.2f;

bool m_keepInterResults;
float m_totalBuildTimeMs;

//// DEFAULTS
float m_agentHeight = 0.01f;  // , 5.0f, 0.1f);
float m_agentRadius = 0.01f;  // , 5.0f, 0.1f);
float m_agentMaxClimb = 0.5f;  // , 5.0f, 0.1f);
float m_agentMaxSlope = 0.5f;  // , 90.0f, 1.0f);

float m_regionMinSize = 1.0f;  // , 150.0f, 1.0f);
float m_regionMergeSize = 1.0f;  // , 150.0f, 1.0f);
bool m_monotonePartitioning = 0;

float m_edgeMaxLen = 50.0f;  // , 50.0f, 1.0f);
float m_edgeMaxError = 1.0f;  // , 3.0f, 0.1f);
float m_vertsPerPoly = 3.0f;  // , 12.0f, 1.0f);		

float m_detailSampleDist = 0.0f;  // , 16.0f, 1.0f);
float m_detailSampleMaxError = 8.0f;  // , 16.0f, 1.0f);

unsigned char* m_triareas;
rcHeightfield* m_solid;
rcCompactHeightfield* m_chf;
rcContourSet* m_cset;
rcPolyMesh* m_pmesh;
rcConfig m_cfg;	
rcPolyMeshDetail* m_dmesh;

/// Tool types.
enum SampleToolType
{
	TOOL_NONE = 0,
	TOOL_TILE_EDIT,
	TOOL_TILE_HIGHLIGHT,
	TOOL_TEMP_OBSTACLE,
	TOOL_NAVMESH_TESTER,
	TOOL_NAVMESH_PRUNE,
	TOOL_OFFMESH_CONNECTION,
	TOOL_CONVEX_VOLUME,
	TOOL_CROWD,
	MAX_TOOLS
};

/// These are just sample areas to use consistent values across the samples.
/// The use should specify these base on his needs.
enum SamplePolyAreas
{
	SAMPLE_POLYAREA_GROUND,
	SAMPLE_POLYAREA_WATER,
	SAMPLE_POLYAREA_ROAD,
	SAMPLE_POLYAREA_DOOR,
	SAMPLE_POLYAREA_GRASS,
	SAMPLE_POLYAREA_JUMP,
};
enum SamplePolyFlags
{
	SAMPLE_POLYFLAGS_WALK		= 0x01,		// Ability to walk (ground, grass, road)
	SAMPLE_POLYFLAGS_SWIM		= 0x02,		// Ability to swim (water).
	SAMPLE_POLYFLAGS_DOOR		= 0x04,		// Ability to move through doors.
	SAMPLE_POLYFLAGS_JUMP		= 0x08,		// Ability to jump.
	SAMPLE_POLYFLAGS_DISABLED	= 0x10,		// Disabled polygon
	SAMPLE_POLYFLAGS_ALL		= 0xffff	// All abilities.
};

enum DrawMode
{
	DRAWMODE_NAVMESH,
	DRAWMODE_NAVMESH_TRANS,
	DRAWMODE_NAVMESH_BVTREE,
	DRAWMODE_NAVMESH_NODES,
	DRAWMODE_NAVMESH_INVIS,
	DRAWMODE_MESH,
	DRAWMODE_VOXELS,
	DRAWMODE_VOXELS_WALKABLE,
	DRAWMODE_COMPACT,
	DRAWMODE_COMPACT_DISTANCE,
	DRAWMODE_COMPACT_REGIONS,
	DRAWMODE_REGION_CONNECTIONS,
	DRAWMODE_RAW_CONTOURS,
	DRAWMODE_BOTH_CONTOURS,
	DRAWMODE_CONTOURS,
	DRAWMODE_POLYMESH,
	DRAWMODE_POLYMESH_DETAIL,
	MAX_DRAWMODE
};

DrawMode m_drawMode;

BuildContext* m_ctx;




////////////////////////////

extern "C" {
  extern char* getThreeJSMeshes();
}

int getAreasCount(int i)
{
	return m_pmesh->npolys;
}
void getAreaVerts(int i)
{
}

void debugConfig()
{
	printf("config \n");
	printf(" m_cellSize=%f \n", m_cellSize);
	printf(" m_cellHeight=%f \n", m_cellHeight);

	//// DEFAULTS
	printf(" m_agentHeight=%f \n", m_agentHeight);
	printf(" m_agentRadius=%f \n", m_agentRadius);
	printf(" m_agentMaxClimb=%f \n", m_agentMaxClimb);
	printf(" m_agentMaxSlope=%f \n", m_agentMaxSlope);

	printf(" m_regionMinSize=%f \n", m_regionMinSize);
	printf(" m_regionMergeSize=%f \n", m_regionMergeSize);

	printf(" m_edgeMaxLen=%f \n", m_edgeMaxLen);
	printf(" m_edgeMaxError=%f \n", m_edgeMaxError);
	printf(" m_vertsPerPoly=%f \n", m_vertsPerPoly);		

	printf(" m_detailSampleDist=%f \n", m_detailSampleDist);
	printf(" m_detailSampleMaxError=%f \n", m_detailSampleMaxError);
}

////////////////////////////

void emscripten_log(char* string, bool escape = true)
{
	char buff[1024];
	sprintf(buff, (escape ? "console.log('%s');" : "console.log(%s);"), string);
	emscripten_run_script(buff);
}
void emscripten_debugger()
{
	emscripten_run_script("debugger");
}

void cleanup()
{
	printf("cleanup \n");
	
	delete [] m_triareas;
	m_triareas = 0;
	rcFreeHeightField(m_solid);
	m_solid = 0;
	rcFreeCompactHeightfield(m_chf);
	m_chf = 0;
	rcFreeContourSet(m_cset);
	m_cset = 0;
	rcFreePolyMesh(m_pmesh);
	m_pmesh = 0;
	rcFreePolyMeshDetail(m_dmesh);
	m_dmesh = 0;
	dtFreeNavMesh(m_navMesh);
	m_navMesh = 0;
}

void bareGeomInit(){
	m_geom = new InputGeom;
	meshLoader = new rcMeshLoaderObj;
}
void bareGeomAddVertex(float x, float y, float z, int cap){
	meshLoader->addVertex(x, y, z, cap);
}
void bareGeomAddTriangle(int a, int b, int c, int cap){
	meshLoader->addTriangle(a, b, c, cap);
}
void bareGeomValidate(){
	m_geom->loadFromMeshLoader(m_ctx, meshLoader);
}
void iterateOnNavMesh(){
	const int nvp = m_pmesh->nvp;
	const float cs = m_pmesh->cs;
	const float ch = m_pmesh->ch;
	const float* orig = m_pmesh->bmin;

	int nIndex = 0;
	char buff[512];

	// window.object.scale.set(10, 10, 10); window.object.position.set(200, 0, -200);
	// window.object.scale.set(2.1, 2.1, 4); window.object.position.setZ(-70)

	// sprintf(buff, "bvQuantFactor=%u", vi[0], vi[1], vi[2]);
	// emscripten_log(buff);

	sprintf(buff, "nvp=%u, cs=%f, ch=%f, orig=%f", nvp, cs, ch, orig);
	emscripten_log(buff);

	emscripten_run_script("window.materials = [ new THREE.MeshPhongMaterial({ color:0xff8888, ambient: 0xff8888, side:THREE.DoubleSide, wireframe:true }) ];");
	emscripten_run_script("window.points = [];");

	for (int i = 0; i < m_pmesh->npolys; ++i)
	{
		if (m_pmesh->areas[i] == SAMPLE_POLYAREA_GROUND)
        {
			const unsigned short* p = &m_pmesh->polys[i*nvp*2];

			unsigned short vi[3];
			for (int j = 2; j < nvp; ++j)
			{
				if (p[j] == RC_MESH_NULL_IDX) break;
				vi[0] = p[0];
				vi[1] = p[j-1];
				vi[2] = p[j];

				// emscripten_debugger();

				// TODO: how to add navmesh model ?

				sprintf(buff, "vi = { x:%u, y:%u, z:%u }", vi[0], vi[1], vi[2]);
				emscripten_log(buff);

				for (int k = 0; k < 3; ++k)
				{
					const unsigned short* v = &m_pmesh->verts[vi[k]*3];
					const float x = orig[0] + v[0]; //*cs;
					const float y = orig[1] + (v[1]+1); //*ch;
					const float z = orig[2] + v[2] - 140; //*cs;

					sprintf(buff, "window.points.push(new THREE.Vector3(%f, %f, %f));", x, y, z);
					emscripten_run_script(buff);

					nIndex += 3;
				}
			}
		}

		//sprintf(buff, "window.object.position.set( %u, %u, %u );", vi[0], vi[1], vi[2]);
		//emscripten_run_script(buff);
		//emscripten_log(buff);
	}
	emscripten_log("window.points.length, ' points'", false);

	emscripten_run_script("window.object = new THREE.Mesh(new THREE.ConvexGeometry(window.points), window.materials);");		

	sprintf(buff, "/* window.object.position.set( %u, %u, %u );  */ window.object.scale.set( new THREE.Vector3( %f, %f, %f ) );  ", 0, 0, 0, 1/cs, 1/ch, 1/cs);
	emscripten_run_script(buff);
	emscripten_log(buff);

	emscripten_run_script("scene.add(window.object);");
	emscripten_log("window.object", false);
}

int getMaxTiles(){
	return m_navMesh->getMaxTiles();
}

void drawNavMeshTiles(){
	for (int i = 0; i < m_navMesh->getMaxTiles(); ++i)
	{
		//dtMeshTile* tile = m_navMesh->getTile(i);
	}
}

void set_cellSize(float val){				m_cellSize = val;				}
void set_cellHeight(float val){				m_cellHeight = val;				}
void set_agentHeight(float val){			m_agentHeight = val;			}
void set_agentRadius(float val){			m_agentRadius = val;			}
void set_agentMaxClimb(float val){			m_agentMaxClimb = val;			}
void set_agentMaxSlope(float val){			m_agentMaxSlope = val;			}
void set_regionMinSize(float val){			m_regionMinSize = val;			}
void set_regionMergeSize(float val){		m_regionMergeSize = val;		}
void set_edgeMaxLen(float val){				m_edgeMaxLen = val;				}
void set_edgeMaxError(float val){			m_edgeMaxError = val;			}
void set_vertsPerPoly(float val){			m_vertsPerPoly = val;			}		
void set_detailSampleDist(float val){		m_detailSampleDist = val;		}
void set_detailSampleMaxError(float val){	m_detailSampleMaxError = val;	}

/////////////////////////////
bool init()
{
	return getThreeJSMeshes();
}

bool initWithFile()
{
	printf("loading from file");
	m_geom = new InputGeom;
	if (!m_geom || !m_geom->loadMesh(m_ctx, "/meshes/nav_test.obj"))
	{
		printf("cannot load OBJ file \n");
		return false;		
	}
	return true;
}

bool build()
{
	if (!m_geom || !m_geom->getMesh())
	{
		printf("buildNavigation: Input mesh is not specified.");
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Input mesh is not specified.");
		return false;
	}
	
	cleanup();

	const float* bmin = m_geom->getMeshBoundsMin();
	const float* bmax = m_geom->getMeshBoundsMax();
	const float* verts = m_geom->getMesh()->getVerts();
	const int nverts = m_geom->getMesh()->getVertCount();
	const int* tris = m_geom->getMesh()->getTris();
	const int ntris = m_geom->getMesh()->getTriCount();
	
	//
	// Step 1. Initialize build config.
	//
	
	// Init build configuration from GUI
	memset(&m_cfg, 0, sizeof(m_cfg));
	m_cfg.cs = m_cellSize;
	m_cfg.ch = m_cellHeight;
	m_cfg.walkableSlopeAngle = m_agentMaxSlope;
	m_cfg.walkableHeight = (int)ceilf(m_agentHeight / m_cfg.ch);
	m_cfg.walkableClimb = (int)floorf(m_agentMaxClimb / m_cfg.ch);
	m_cfg.walkableRadius = (int)ceilf(m_agentRadius / m_cfg.cs);
	m_cfg.maxEdgeLen = (int)(m_edgeMaxLen / m_cellSize);
	m_cfg.maxSimplificationError = m_edgeMaxError;
	m_cfg.minRegionArea = (int)rcSqr(m_regionMinSize);		// Note: area = size*size
	m_cfg.mergeRegionArea = (int)rcSqr(m_regionMergeSize);	// Note: area = size*size
	m_cfg.maxVertsPerPoly = (int)m_vertsPerPoly;
	m_cfg.detailSampleDist = m_detailSampleDist < 0.9f ? 0 : m_cellSize * m_detailSampleDist;
	m_cfg.detailSampleMaxError = m_cellHeight * m_detailSampleMaxError;
	
	// Set the area where the navigation will be build.
	// Here the bounds of the input mesh are used, but the
	// area could be specified by an user defined box, etc.
	rcVcopy(m_cfg.bmin, bmin);
	rcVcopy(m_cfg.bmax, bmax);
	rcCalcGridSize(m_cfg.bmin, m_cfg.bmax, m_cfg.cs, &m_cfg.width, &m_cfg.height);

	// Reset build times gathering.
	//m_ctx->resetTimers();

	printf("resetTimers \n");

	// Start the build process.	
	//m_ctx->startTimer(RC_TIMER_TOTAL);
	
	m_ctx->log(RC_LOG_PROGRESS, "Building navigation:");
	m_ctx->log(RC_LOG_PROGRESS, " - %d x %d cells", m_cfg.width, m_cfg.height);
	m_ctx->log(RC_LOG_PROGRESS, " - %.1fK verts, %.1fK tris", nverts/1000.0f, ntris/1000.0f);
	
	printf("Building navigation \n");

	//
	// Step 2. Rasterize input polygon soup.
	//
	
	// Allocate voxel heightfield where we rasterize our input data to.
	m_solid = rcAllocHeightfield();
	if (!m_solid)
	{
		printf("buildNavigation: Out of memory 'solid'. \n");
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'solid'.");
		return false;
	}
	if (!rcCreateHeightfield(m_ctx, *m_solid, m_cfg.width, m_cfg.height, m_cfg.bmin, m_cfg.bmax, m_cfg.cs, m_cfg.ch))
	{
		printf("buildNavigation: Could not create solid heightfield. \n");
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not create solid heightfield.");
		return false;
	}
	
	printf("Heightfield polygon soup \n");

	// Allocate array that can hold triangle area types.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	m_triareas = new unsigned char[ntris];
	if (!m_triareas)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'm_triareas' (%d).", ntris);
		return false;
	}
	
	// Find triangles which are walkable based on their slope and rasterize them.
	// If your input data is multiple meshes, you can transform them here, calculate
	// the are type for each of the meshes and rasterize them.
	memset(m_triareas, 0, ntris*sizeof(unsigned char));
	rcMarkWalkableTriangles(m_ctx, m_cfg.walkableSlopeAngle, verts, nverts, tris, ntris, m_triareas);
	printf("%u Walkable Triangles\n", sizeof(m_triareas));

	rcRasterizeTriangles(m_ctx, verts, nverts, tris, m_triareas, ntris, *m_solid, m_cfg.walkableClimb);

	if (!m_keepInterResults)
	{
		delete [] m_triareas;
		m_triareas = 0;
	}
	
	//
	// Step 3. Filter walkables surfaces.
	//
	
	// Once all geoemtry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	rcFilterLowHangingWalkableObstacles(m_ctx, m_cfg.walkableClimb, *m_solid);
	rcFilterLedgeSpans(m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid);
	rcFilterWalkableLowHeightSpans(m_ctx, m_cfg.walkableHeight, *m_solid);

	printf("filters \n");
	
	//
	// Step 4. Partition walkable surface to simple regions.
	//

	// Compact the heightfield so that it is faster to handle from now on.
	// This will result more cache coherent data as well as the neighbours
	// between walkable cells will be calculated.
	m_chf = rcAllocCompactHeightfield();
	if (!m_chf)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'chf'.");
		return false;
	}
	if (!rcBuildCompactHeightfield(m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid, *m_chf))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build compact data.");
		return false;
	}
	
	if (!m_keepInterResults)
	{
		rcFreeHeightField(m_solid);
		m_solid = 0;
	}
		
	// Erode the walkable area by agent radius.
	if (!rcErodeWalkableArea(m_ctx, m_cfg.walkableRadius, *m_chf))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not erode.");
		return false;
	}

	// (Optional) Mark areas.
	const ConvexVolume* vols = m_geom->getConvexVolumes();
	for (int i  = 0; i < m_geom->getConvexVolumeCount(); ++i)
		rcMarkConvexPolyArea(m_ctx, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area, *m_chf);
	
	if (m_monotonePartitioning)
	{
		// Partition the walkable surface into simple regions without holes.
		// Monotone partitioning does not need distancefield.
		if (!rcBuildRegionsMonotone(m_ctx, *m_chf, 0, m_cfg.minRegionArea, m_cfg.mergeRegionArea))
		{
			m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build regions.");
			return false;
		}
	}
	else
	{
		// Prepare for region partitioning, by calculating distance field along the walkable surface.
		if (!rcBuildDistanceField(m_ctx, *m_chf))
		{
			m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build distance field.");
			return false;
		}

		// Partition the walkable surface into simple regions without holes.
		if (!rcBuildRegions(m_ctx, *m_chf, 0, m_cfg.minRegionArea, m_cfg.mergeRegionArea))
		{
			m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build regions.");
			return false;
		}
	}

	printf("Partition walkable surface \n");

	//
	// Step 5. Trace and simplify region contours.
	//
	
	// Create contours.
	m_cset = rcAllocContourSet();
	if (!m_cset)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'cset'.");
		return false;
	}
	if (!rcBuildContours(m_ctx, *m_chf, m_cfg.maxSimplificationError, m_cfg.maxEdgeLen, *m_cset))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not create contours.");
		return false;
	}
	
	printf("Trace and simplify region contours: %u conts (maxSimplificationError= %f, maxEdgeLen= %u)\n", m_cset->nconts, m_cfg.maxSimplificationError, m_cfg.maxEdgeLen);

	//
	// Step 6. Build polygons mesh from contours.
	//
	
	// Build polygon navmesh from the contours.
	m_pmesh = rcAllocPolyMesh();
	if (!m_pmesh)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmesh'.");
		return false;
	}
	if (!rcBuildPolyMesh(m_ctx, *m_cset, m_cfg.maxVertsPerPoly, *m_pmesh))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not triangulate contours.");
		return false;
	}
	
	printf("Build polygons mesh from contours. \n");

	//
	// Step 7. Create detail mesh which allows to access approximate height on each polygon.
	//
	
	m_dmesh = rcAllocPolyMeshDetail();
	if (!m_dmesh)
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmdtl'.");
		return false;
	}

	if (!rcBuildPolyMeshDetail(m_ctx, *m_pmesh, *m_chf, m_cfg.detailSampleDist, m_cfg.detailSampleMaxError, *m_dmesh))
	{
		m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not build detail mesh.");
		return false;
	}

	if (!m_keepInterResults)
	{
		rcFreeCompactHeightfield(m_chf);
		m_chf = 0;
		rcFreeContourSet(m_cset);
		m_cset = 0;
	}

	printf("Create detail mesh. \n");

	// At this point the navigation mesh data is ready, you can access it from m_pmesh.
	// See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.
	
	//
	// (Optional) Step 8. Create Detour data from Recast poly mesh.
	//
	
	// The GUI may allow more max points per polygon than Detour can handle.
	// Only build the detour navmesh if we do not exceed the limit.
	if (m_cfg.maxVertsPerPoly <= DT_VERTS_PER_POLYGON)
	{
		unsigned char* navData = 0;
		int navDataSize = 0;

		printf("%u poly areas \n", m_pmesh->npolys);

		// Update poly flags from areas.
		for (int i = 0; i < m_pmesh->npolys; ++i)
		{
			//printf("Update poly flags from area %u \n", i);

			if (m_pmesh->areas[i] == RC_WALKABLE_AREA)
				m_pmesh->areas[i] = SAMPLE_POLYAREA_GROUND;
				
			if (m_pmesh->areas[i] == SAMPLE_POLYAREA_GROUND ||
				m_pmesh->areas[i] == SAMPLE_POLYAREA_GRASS ||
				m_pmesh->areas[i] == SAMPLE_POLYAREA_ROAD)
			{
				m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
			}
			else if (m_pmesh->areas[i] == SAMPLE_POLYAREA_WATER)
			{
				m_pmesh->flags[i] = SAMPLE_POLYFLAGS_SWIM;
			}
			else if (m_pmesh->areas[i] == SAMPLE_POLYAREA_DOOR)
			{
				m_pmesh->flags[i] = SAMPLE_POLYFLAGS_WALK | SAMPLE_POLYFLAGS_DOOR;
			}
		}


		dtNavMeshCreateParams params;
		memset(&params, 0, sizeof(params));
		params.verts = m_pmesh->verts;
		params.vertCount = m_pmesh->nverts;
		params.polys = m_pmesh->polys;
		params.polyAreas = m_pmesh->areas;
		params.polyFlags = m_pmesh->flags;
		params.polyCount = m_pmesh->npolys;
		params.nvp = m_pmesh->nvp;
		params.detailMeshes = m_dmesh->meshes;
		params.detailVerts = m_dmesh->verts;
		params.detailVertsCount = m_dmesh->nverts;
		params.detailTris = m_dmesh->tris;
		params.detailTriCount = m_dmesh->ntris;
		params.offMeshConVerts = m_geom->getOffMeshConnectionVerts();
		params.offMeshConRad = m_geom->getOffMeshConnectionRads();
		params.offMeshConDir = m_geom->getOffMeshConnectionDirs();
		params.offMeshConAreas = m_geom->getOffMeshConnectionAreas();
		params.offMeshConFlags = m_geom->getOffMeshConnectionFlags();
		params.offMeshConUserID = m_geom->getOffMeshConnectionId();
		params.offMeshConCount = m_geom->getOffMeshConnectionCount();
		params.walkableHeight = m_agentHeight;
		params.walkableRadius = m_agentRadius;
		params.walkableClimb = m_agentMaxClimb;
		rcVcopy(params.bmin, m_pmesh->bmin);
		rcVcopy(params.bmax, m_pmesh->bmax);
		params.cs = m_cfg.cs;
		params.ch = m_cfg.ch;
		params.buildBvTree = true;

		printf("Get NavMeshCreateParams %p \n", params);
		
		if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
		{
			m_ctx->log(RC_LOG_ERROR, "Could not build Detour navmesh.");
			return false;
		}
		
		printf("Built Detour navdata. %p \n", navData);

		m_navMesh = dtAllocNavMesh();
		if (!m_navMesh)
		{
			dtFree(navData);
			m_ctx->log(RC_LOG_ERROR, "Could not create Detour navmesh");
			return false;
		}

		printf("Created Detour navmesh. %p \n", m_navMesh);

		dtStatus status;
		
		status = m_navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
		if (dtStatusFailed(status))
		{
			dtFree(navData);
			m_ctx->log(RC_LOG_ERROR, "Could not init Detour navmesh");
			return false;
		}
		
		printf("Init Detour navmesh. %p \n", navData);

		status = m_navQuery->init(m_navMesh, 2048);
		if (dtStatusFailed(status))
		{
			m_ctx->log(RC_LOG_ERROR, "Could not init Detour navmesh query");
			return false;
		}
	}

	printf("m_navMesh=%p \n", m_navMesh);
	
	//m_ctx->stopTimer(RC_TIMER_TOTAL);

	// Show performance stats.
	//duLogBuildTimes(*m_ctx, m_ctx->getAccumulatedTime(RC_TIMER_TOTAL));
	//m_ctx->log(RC_LOG_PROGRESS, ">> Polymesh: %d vertices  %d polygons", m_pmesh->nverts, m_pmesh->npolys);
	
	//m_totalBuildTimeMs = m_ctx->getAccumulatedTime(RC_TIMER_TOTAL)/1000.0f;

	return true;
}



#include <emscripten/bind.h>

using namespace emscripten;

EMSCRIPTEN_BINDINGS(my_module) {
    function("init", &init);
    function("initWithFile", &initWithFile);

    function("build", &build);

    function("bareGeomInit", &bareGeomInit);
	function("bareGeomAddVertex", &bareGeomAddVertex);
	function("bareGeomAddTriangle", &bareGeomAddTriangle);
	function("bareGeomValidate", &bareGeomValidate);

	function("iterateOnNavMesh", &iterateOnNavMesh);

	function("set_cellSize", &set_cellSize);
	function("set_cellHeight", &set_cellHeight);
	function("set_agentHeight", &set_agentHeight);
	function("set_agentRadius", &set_agentRadius);
	function("set_agentMaxClimb", &set_agentMaxClimb);
	function("set_agentMaxSlope", &set_agentMaxSlope);
	function("set_regionMinSize", &set_regionMinSize);
	function("set_regionMergeSize", &set_regionMergeSize);
	function("set_edgeMaxLen", &set_edgeMaxLen);
	function("set_edgeMaxError", &set_edgeMaxError);
	function("set_vertsPerPoly", &set_vertsPerPoly);
	function("set_detailSampleDist", &set_detailSampleDist);
	function("set_detailSampleMaxError", &set_detailSampleMaxError);

	function("getMaxTiles", &getMaxTiles);

}
