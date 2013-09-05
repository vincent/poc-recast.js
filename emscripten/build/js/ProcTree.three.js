/*
proctree.js
Copyright (c) 2012, Paul Brunt
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of tree.js nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL PAUL BRUNT BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
(function(env){
    env.Tree.prototype.asThreeGeometry = function() {

		var i, tree = new THREE.Geometry();

		for (i = 0; i < this.verts.length; i++) {
			tree.vertices.push(new THREE.Vector3( this.verts[i][0], this.verts[i][1], this.verts[i][2] ));
		}
		var face, normal, uv1, uv2, uv3;
		for (i = 0; i < this.faces.length && i < this.normals.length; i++) {
			face = new THREE.Face3( this.faces[i][0], this.faces[i][1], this.faces[i][2] );
			normal = this.normals[i];
			face.normal.set(normal[0], normal[1], normal[2]); // normal
			tree.faces.push(face);
		}

		for (i = 0; i < this.UV.length; i+=3) {
			uv1 = this.UV[i];
			uv2 = this.UV[i+1];
			uv3 = this.UV[i+2];
			tree.faceVertexUvs[0].push([
				new THREE.Vector2(uv1[0], uv1[1]),
				new THREE.Vector2(uv1[0], uv1[1]),
				new THREE.Vector2(uv1[0], uv1[1])
			]); // uvs
		}


		tree.__dirtyVertices = true;
		tree.__dirtyNormals = true;

		//tree.computeFaceNormals();
		//tree.computeVertexNormals();
		//tree.uvsNeedUpdate = true;
		//tree.verticesNeedUpdate = true;
		//tree.normalsNeedUpdate = true;
		return tree;

		var face = new THREE.Face3(2,3,1);
		face.normal.set(0,0,1); // normal
		avatarGeom.faces.push(face);
		avatarGeom.faceVertexUvs[0].push([new THREE.UV(1,1),new THREE.UV(0,1),new THREE.UV(1,0)]); // uvs

		face = new THREE.Face3(0,2,1);
		face.normal.set(0,0,1); // normal
		avatarGeom.faces.push(face);
		avatarGeom.faceVertexUvs[0].push([new THREE.UV(0,0),new THREE.UV(1,1),new THREE.UV(1,0)]); // uvs
    };


})(window);