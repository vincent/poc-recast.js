Proof of concept
===

> recast.js is an [Emscripten](https://github.com/kripken/emscripten) experiment to use the [RecastDetour](https://code.google.com/p/recastnavigation) in JavaScript

Goal
===

To use RecastDetour primitives against a mesh, likely a Three.js mesh 


Howto
===

* have a running Emscripten environement
* build emscripten output with ```shell ./build.sh recast.emcc.html```
* build library bootstrap with ```shell ./build.sh recast.emcc.js``` and open ```build/_webgl_loader_obj.html```


