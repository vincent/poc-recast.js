bool NavMesher::Build()
{
  // ******* Only for OBJ Loading ****
 cleanup();
 const char * filepath = "../../media/models/";
 if (!m_geom || !m_geom->loadMesh(filepath))
 {
  delete m_geom;
  m_geom = 0;
  
  m_ctx->log(RC_LOG_ERROR, "Geom load log %s:");
 }
 assert(m_geom);
 if (!m_geom || !m_geom->getMesh())
 {
  m_ctx->log(RC_LOG_ERROR, "buildNavigation: Input mesh is not specified.");
  return false;
 }
 if(m_geom->getMesh()->getTriCount() <= 0 || m_geom->getMesh()->getVertCount()<=0)
  Ogre::Exception(0,Ogre::String("Bad verts or Triangle count. Verts: "+
  StringConverter::toString( m_geom->getMesh()->getVertCount()) + "/n"
  + "Triangles :" +StringConverter::toString(m_geom->getMesh()->getTriCount())),"NavMesher::Build");
 
 //reset timer
 Ogre::Timer tm;
 tm.reset();
 unsigned long stime = tm.getMicroseconds();
 //clear existing
 Clear();
  // ******* Only for OBJ Loading ****
 const float* bmin = m_geom->getMeshBoundsMin();
 const float* bmax = m_geom->getMeshBoundsMax();
 const float* verts = m_geom->getMesh()->getVerts();
 const int nverts = m_geom->getMesh()->getVertCount();
 const int *tris = m_geom->getMesh()->getTris();
 const int ntris = m_geom->getMesh()->getTriCount();

 if(sizeof(tris) <= 0 || ntris <= 0) {
  return false;
 }
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
 m_cfg.minRegionArea = (int)rcSqr(m_regionMinSize);  // Note: area = size*size
 m_cfg.mergeRegionArea = (int)rcSqr(m_regionMergeSize); // Note: area = size*size
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
 m_ctx->resetTimers();
 // Start the build process. 
 m_ctx->startTimer(RC_TIMER_TOTAL);
 
 m_ctx->log(RC_LOG_PROGRESS, "Building navigation:");
 m_ctx->log(RC_LOG_PROGRESS, " - %d x %d cells", m_cfg.width, m_cfg.height);
 m_ctx->log(RC_LOG_PROGRESS, " - %.1fK verts, %.1fK tris", nverts/1000.0f, ntris/1000.0f);
 
 //
 // Step 2. Rasterize input polygon soup.
 //
 
 // Allocate voxel heightfield where we rasterize our input data to.
 m_solid = rcAllocHeightfield();
 if (!m_solid)
 {
  m_ctx->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'solid'.");
  return false;
 }
 if (!rcCreateHeightfield(m_ctx, *m_solid, m_cfg.width, m_cfg.height, m_cfg.bmin, m_cfg.bmax, m_cfg.cs, m_cfg.ch))
 {
  m_ctx->log(RC_LOG_ERROR, "buildNavigation: Could not create solid heightfield.");
  return false;
 }
 
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

 // At this point the navigation mesh data is ready, you can access it from m_pmesh.
 // See rcDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.
 
 //
 // (Optional) Step 8. Create Detour data from Recast poly mesh.
 //
 
 // The GUI may allow more max points per polygon than Detour can handle.
 // Only build the detour navmesh if we do not exceed the limit.
 unsigned char* navData = 0;
 int navDataSize = 0;
 // Update poly flags from areas.

 for (int i = 0; i < m_pmesh->npolys; ++i)
 {
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

 memset(&m_params, 0, sizeof(m_params));
 m_params.verts = m_pmesh->verts;
 m_params.vertCount = m_pmesh->nverts;
 m_params.polys = m_pmesh->polys;
 m_params.polyAreas = m_pmesh->areas;
 m_params.polyFlags = m_pmesh->flags;
 m_params.polyCount = m_pmesh->npolys;
 m_params.nvp = m_pmesh->nvp;
 m_params.detailMeshes = m_dmesh->meshes;
 m_params.detailVerts = m_dmesh->verts;
 m_params.detailVertsCount = m_dmesh->nverts;
 m_params.detailTris = m_dmesh->tris;
 m_params.detailTriCount = m_dmesh->ntris;
 m_params.walkableHeight = m_agentHeight;
 m_params.walkableRadius = m_agentRadius;
 m_params.walkableClimb = m_agentMaxClimb;
 rcVcopy(m_params.bmin, m_pmesh->bmin);
 rcVcopy(m_params.bmax, m_pmesh->bmax);
 m_params.cs = m_cfg.cs;
 m_params.ch = m_cfg.ch;
 m_params.buildBvTree = true;
 if (!dtCreateNavMeshData(&m_params, &navData, &navDataSize)) {
   m_ctx->log(RC_LOG_ERROR, "Could not build Detour navmesh.");
  return false;
 }
 
 m_navMesh = dtAllocNavMesh();
 if (!m_navMesh) {
  delete [] navData;
   m_ctx->log(RC_LOG_ERROR, "Could not create Detour navmesh");
  return false;
 }
 m_navQuery = dtAllocNavMeshQuery(); 
 dtStatus status = m_navQuery->init(m_navMesh, 2048);
   if (dtStatusFailed(status)) {
           m_ctx->log(RC_LOG_ERROR, "Could not init Detour navmesh query");
           return false;
   }
 if (!m_navMesh->init(navData, navDataSize, true)) {
  delete [] navData;
   m_ctx->log(RC_LOG_ERROR, "Could not init Detour navmesh");
  return false;
 }
 //take time
 stime = tm.getMicroseconds() - stime;
 DrawDebug();
 return true;
}