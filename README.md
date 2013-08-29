Proof of concept [demo](https://rawgithub.com/vincent/poc-recast.js/master/emscripten/build/demo.html)
===

> recast.js is an [Emscripten](https://github.com/kripken/emscripten) experiment to use the [RecastDetour](https://code.google.com/p/recastnavigation) in JavaScript

Goal
===

To use RecastDetour primitives against a mesh, likely a Three.js mesh 

Whooaa ! Is it working ?
===

Heighmap, NavigationMesh and Pathfinding are working.

Howto
===

* have a running Emscripten environement
* build library bootstrap with ```./build.sh build/recast.emcc.js``` 
 - open ```build/demo.html``` or ```build/demo_player.html```
 - click on ```init```
 - right-click on the mesh to define start and destination


