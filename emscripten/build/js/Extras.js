
Extras = {};



Extras.Pool = function() {
	this.__pools = [];
};

// Get a new Vector
Extras.Pool.prototype.get = function() {
	if ( this.__pools.length > 0 ) {
		return this.__pools.pop();
	}

	console.log( "pool ran out!" )
	return null;
};

// Release a vector back into the pool
Extras.Pool.prototype.add = function( v ) {
	this.__pools.push( v );
};



Extras.ParticleCloud = function( length, texture, light, options ) {

	var options = options || {};
	this.length = length;
	this.light = light;

	this.delta = 0;
	this.pool = new Extras.Pool();

	this.colorSL = options.colorSL || [ 0.6, 0.1 ];

	this.colorHSL = options.colorHSL || new THREE.Color( 0xffffff );

	//////////////////////////////

	this._timeOnShapePath = 0;

	this.particles = new THREE.Geometry();

	for (var i = 0; i < this.length; i ++ ) {

		this.particles.vertices.push( new THREE.Vector3( 1, 1, 1 ) );
		this.pool.add( i );
	
	}

	this.attributes = {
		
		size:  { type: 'f', value: [] },
		pcolor: { type: 'c', value: [] }

	};

	this.uniforms = {
		texture:   { type: "t", value: texture }
	};

	this.shaderMaterial = new THREE.ShaderMaterial( {

		uniforms: this.uniforms,
		attributes: this.attributes,

		vertexShader: document.getElementById( 'vertexshader' ).textContent,
		fragmentShader: document.getElementById( 'fragmentshader' ).textContent,

		blending: THREE.AdditiveBlending,
		depthWrite: false,
		transparent: true

	});

	this.particleCloud = new THREE.ParticleSystem( this.particles, this.shaderMaterial );

	this.particleCloud.dynamic = true;
	// this.particleCloud.sortParticles = true;

	this.values_size = this.attributes.size.value;
	this.values_color = this.attributes.pcolor.value;

	for( var v = 0; v < this.particleCloud.geometry.vertices.length; v ++ ) {

		this.values_size[ v ] = 1;

		this.values_color[ v ] = new THREE.Color( 0x000000 );

		this.particles.vertices[ v ].set( Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY );

	}

	// EMITTER STUFF

	function bind( scope, fn ) {
		return function () {
			return fn.apply( scope, arguments );
		};
	};

	this.hue = 0;

	this.sparksEmitter = new SPARKS.Emitter( new SPARKS.SteadyCounter( 50 ) );

	this.emitterpos = new THREE.Vector3( 0, 0, 0 );

	this.sparksEmitter.addInitializer( new SPARKS.Position( new SPARKS.PointZone( this.emitterpos ) ) );
	this.sparksEmitter.addInitializer( new SPARKS.Lifetime( .5, 1 ));
	this.sparksEmitter.addInitializer( new SPARKS.Target( null, bind( this, this.setTargetParticle ) ) );


	this.sparksEmitter.addInitializer( new SPARKS.Velocity( new SPARKS.PointZone( new THREE.Vector3( 1, 0, 1 ) ) ) );
	// TOTRY Set velocity to move away from centroid

	this.sparksEmitter.addAction( new SPARKS.Age() );
	this.sparksEmitter.addAction( new SPARKS.Accelerate( 0, 1, 0 ) );
	this.sparksEmitter.addAction( new SPARKS.Move() );
	this.sparksEmitter.addAction( new SPARKS.RandomDrift( 0, 2, 0 ) );

	this.sparksEmitter.addCallback( "created", bind( this, this.onParticleCreated ) );
	this.sparksEmitter.addCallback( "dead", bind( this, this.onParticleDead ) );
};

Extras.ParticleCloud.prototype.setTargetParticle = function() {

	var target = this.pool.get();
	this.values_size[ target ] = Math.random() * 200 + 10;

	return target;
};

Extras.ParticleCloud.prototype.onParticleCreated = function( p ) {

	var position = p.position;

	var target = p.target;

	if ( target ) {
		p.target.position = position;
	
		this.hue += 0.0003 * this.delta;
		if ( this.hue > 1 ) this.hue -= 1;

		// we have a shape to follow
		if (this.shape) {

			this._timeOnShapePath += this.delta;
			if ( this._timeOnShapePath > 1 ) this._timeOnShapePath -= 1;

			var pointOnShape = this.shape.getPointAt( this._timeOnShapePath );

			if (pointOnShape) {
				this.emitterpos.x = pointOnShape.x * 1;// - 100;
				this.emitterpos.z = -pointOnShape.y * 1;// + 400;
			}
		}

		this.particles.vertices[ target ] = p.position;

		// this.values_color[ target ].setHSL( this.hue, 0.6, 0.1 ).multiply( this.colorHSL );
		// this.values_color[ target ].set( this.colorHSL ); // .setHSL( this.hue, 0.6, 0.1 );
		// this.values_color[ target ].multiplyScalar( this.colorHSL );

		
		if (this.light) {
			this.light.color.setHSL( this.hue, 0.8, 0.5 );
		}
	};
};

Extras.ParticleCloud.prototype.onParticleDead = function( particle ) {

	var target = particle.target;

	if ( target ) {
		// Hide the particle
		this.values_color[ target ].setRGB( 0, 0, 0 );
		this.particles.vertices[ target ].set( Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY );

		// Mark particle system as available by returning to pool
		this.pool.add( particle.target );
	}
};

Extras.ParticleCloud.prototype.engineLoopUpdate = function() {

};

Extras.ParticleCloud.prototype.start = function () {

	this.sparksEmitter.start();
};

Extras.ParticleCloud.prototype.stop = function () {

	this.sparksEmitter.stop();
};

Extras.ParticleCloud.prototype.destroy = function () {

	delete this.sparksEmitter;
};

Extras.ParticleCloud.prototype.update = function ( delta ) {

	this.delta = delta;
	this.particleCloud.geometry.verticesNeedUpdate = true;
	this.attributes.size.needsUpdate = true;
	this.attributes.pcolor.needsUpdate = true;
};




Extras.Aura = function( geometry, particulesCount, texture, light ) {

	// Create particle objects for Three.js
	var cloud = new Extras.ParticleCloud( particulesCount, texture, light );

	function circle ( radius, segments ) {
		var circle = new THREE.Shape();
		var radius = radius;

		for (var i = 0; i < segments; i++) {
		  var pct = (i + 1) / segments;
		  var theta = pct * Math.PI * 2.0;
		  var x = radius * Math.cos(theta);
		  var y = radius * Math.sin(theta);
		  if (i == 0) {
		    circle.moveTo(x, y);
		  } else {
		    circle.lineTo(x, y);
		  }
		}
		return circle;
	}

	switch (geometry) {
		case 'point':
			cloud.shape = circle( 0.5, 3);
			break;

		default:
		case 'circle':
			cloud.shape = circle( 6, 17);
			break;
	}


	return cloud;
};


