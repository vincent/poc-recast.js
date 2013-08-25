Proof of concept
===

> recast.js is an emscripten experiment to use the RecastDetour in JavaScript

Goal
===

To use RecastDetour primitives against a mesh, likely a Three.js mesh 

Howto
===

* have a running Emscripten environement
* build emscripten output with ```shell ./build.sh recast.emcc.html```
* build library bootstrap with ```shell ./build.sh recast.emcc.js``` and open ```build/_webgl_loader_obj.html```


