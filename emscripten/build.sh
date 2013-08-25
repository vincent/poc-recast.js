#!/bin/sh
 
export CC='/Users/vincent/Workspace/emscripten/emcc'
export CFLAGS='--closure 0 -s LINKABLE=0 --bind'
export DEFINES='-D NOT_GCC -D EMSCRIPTEN -D USES_UNIX_DIR'
export INCLUDES='-I ../src_cpp/Recast/Include 			
				 -I ../src_cpp/Detour/Include 			
				 -I ../src_cpp/DetourCrowd/Include 			
				 -I ../src_cpp/RecastDemo/Include 		
				 -I ../src_cpp/DebugUtils/Include 		
				 -I ../src_cpp/DetourTileCache/Include'
export FILES='../src_cpp/DebugUtils/Source/DebugDraw.cpp
			  ../src_cpp/DebugUtils/Source/DetourDebugDraw.cpp
			  ../src_cpp/DebugUtils/Source/RecastDebugDraw.cpp
			  ../src_cpp/DebugUtils/Source/RecastDump.cpp
			  ../src_cpp/Detour/Source/DetourAlloc.cpp
			  ../src_cpp/Detour/Source/DetourCommon.cpp
			  ../src_cpp/Detour/Source/DetourNavMesh.cpp
			  ../src_cpp/Detour/Source/DetourNavMeshBuilder.cpp
			  ../src_cpp/Detour/Source/DetourNavMeshQuery.cpp
			  ../src_cpp/Detour/Source/DetourNode.cpp
			  ../src_cpp/DetourCrowd/Source/DetourCrowd.cpp
			  ../src_cpp/DetourCrowd/Source/DetourLocalBoundary.cpp
			  ../src_cpp/DetourCrowd/Source/DetourObstacleAvoidance.cpp
			  ../src_cpp/DetourCrowd/Source/DetourPathCorridor.cpp
			  ../src_cpp/DetourCrowd/Source/DetourPathQueue.cpp
			  ../src_cpp/DetourCrowd/Source/DetourProximityGrid.cpp
			  ../src_cpp/DetourTileCache/Source/DetourTileCache.cpp
			  ../src_cpp/DetourTileCache/Source/DetourTileCacheBuilder.cpp
			  ../src_cpp/Recast/Source/Recast.cpp
			  ../src_cpp/Recast/Source/RecastAlloc.cpp
			  ../src_cpp/Recast/Source/RecastArea.cpp
			  ../src_cpp/Recast/Source/RecastContour.cpp
			  ../src_cpp/Recast/Source/RecastFilter.cpp
			  ../src_cpp/Recast/Source/RecastLayers.cpp
			  ../src_cpp/Recast/Source/RecastMesh.cpp
			  ../src_cpp/Recast/Source/RecastMeshDetail.cpp
			  ../src_cpp/Recast/Source/RecastRasterization.cpp
			  ../src_cpp/Recast/Source/RecastRegion.cpp
			  ../src_cpp/RecastDemo/Source/ChunkyTriMesh.cpp
			  ../src_cpp/RecastDemo/Source/Filelist.cpp
			  ../src_cpp/RecastDemo/Source/InputGeom.cpp
			  ../src_cpp/RecastDemo/Source/MeshLoaderObj.cpp
			  ../src_cpp/RecastDemo/Source/PerfTimer.cpp
			  ../src_cpp/RecastDemo/Source/Sample.cpp
			  ../src_cpp/RecastDemo/Source/Sample_Debug.cpp
			  ../src_cpp/RecastDemo/Source/Sample_SoloMesh.cpp
			  ../src_cpp/RecastDemo/Source/Sample_TileMesh.cpp
			  ../src_cpp/RecastDemo/Source/SampleInterfaces.cpp
			  ../src_cpp/RecastDemo/Source/ValueHistory.cpp
			  source/main.cpp'

#			  ../src_cpp/RecastDemo/Source/Sample_TempObstacles.cpp

export FLAGS='-v'
export PRELOAD='--preload-file meshes/nav_test.obj'
 
mkdir -p build
 
$CC $FLAGS $DEFINES $INCLUDES $CFLAGS $FILES -s EXPORTED_FUNCTIONS='[]' -o $1 $PRELOAD