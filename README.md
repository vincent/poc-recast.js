Proof of concept
===

> recast.js is an [Emscripten](https://github.com/kripken/emscripten) experiment to use the [RecastDetour](https://code.google.com/p/recastnavigation) in JavaScript

Goal
===

To use RecastDetour primitives against a mesh, likely a Three.js mesh 

Howto
===

* have a running Emscripten environement
* build library bootstrap with ```shell ./build.sh recast.emcc.js``` 
 - open ```build/_webgl_loader_obj.html```
 - use the preloaded file test_nav.obj with ```initWithFile```
 - build the heightmap and stuff with ```initWithFile```
 - draw the navigation mesh with ```initWithFile```, this is the ~ equivalent of ```rcDebugDrawPolyMesh```


