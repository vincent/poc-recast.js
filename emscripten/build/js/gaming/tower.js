
// LOAD_MANAGER

var THREEGAME = THREEGAME || {
	CHARACTERS: [],
};

THREEGAME.DefenseTower = function( x, y, z, options ) {

	THREE.Object3D.apply( this );

	this.bulletSpeed = 100;

	/////

	this._firing = false;
	this._currentTweens = [];

	var self = this,

		x = x ||  0,
		y = y || 28,
		z = z ||  1,
		options = options || {}
		;

	options.start = options.start || true;
	options.transform = options.transform || function( obj3d ){ return obj3d },
	options.fireIntensity = options.fireIntensity || 20000;
	options.texture = options.texture || THREE.ImageUtils.loadTexture( "textures/lensflare2.jpg" );


	var loader = new THREE.ColladaLoader();
	loader.load( 'obj/dota/lantern.dae', function ( loaded ) {

		options.transform( loaded );

	    self.aura = Extras.Aura( 'point', options.fireIntensity, options.texture, pointLight );
	    self.aura.particleCloud.position.set( x, y, z );
	    self.add( self.aura.particleCloud );

		self.add( loaded.scene.children[0] );

		var selfUpdate = _.bind( self.update, self );

	    if ( options.start ) {
	        self.aura.start();
	        container.addEventListener( 'render-update', selfUpdate );
	    }
	});
};
THREEGAME.DefenseTower.prototype = new THREE.Object3D();

THREEGAME.DefenseTower.prototype.update = function( event ) {

	var self = this;

	if ( this.aura ) {
		this.aura.update( event.detail.delta );
	}

	if (this._firing) return;

	_.each( THREEGAME.CHARACTERS, function( c ) {
		if ( c.position.distanceTo( self.position ) < 70 ) {

			self.fireTo( c );

		}
	});

};

THREEGAME.DefenseTower.prototype.stopFiring = function( target ) {
	this._firing = false;
};

THREEGAME.DefenseTower.prototype.fireTo = function( target ) {

	if (this._firing) return;
    this._firing = true;


	console.log('prepare bullet');
	return;
	
	var startPosition = this.position.clone();
	startPosition.y += 28;

	var self = this,
		line = new THREE.SplineCurve3([ startPosition, target.position ]),
		cloud = new Extras.ParticleCloud( 10000, THREE.ImageUtils.loadTexture( "textures/lensflare/lensflare0_alpha.png" ) ),
		cloudUpdate = _.bind( function(event){ cloud.update(event.detail.delta); }, cloud )
		;

	var tween = new TWEEN.Tween({ distance: 0 })

        .to( { distance: 1 }, line.getLength() * 10 ) // use 

        .easing(TWEEN.Easing.Linear.None)

        .onStart(function(){
	        container.addEventListener( 'render-update', cloudUpdate );

			self.add( cloud.particleCloud );
			cloud.start();

			console.log('fire');

            setTimeout( function(){
				console.log('timeout');
				tween.stop();
		        container.removeEventListener( 'render-update', cloudUpdate );

	            self._firing = false;
				self.remove( cloud.particleCloud );
				cloud.stop();
				cloud.destroy();

            }, 1000 );
        })
        
        .onComplete(function(){
	        container.removeEventListener( 'render-update', cloudUpdate );

            self._firing = false;
			self.remove( cloud.particleCloud );
			cloud.stop();
			cloud.destroy();

			console.log('stop');
        })
        
        .onUpdate(function(){
			// get the position data half way along the path
			var pathPosition = line.getPoint(this.distance);

			// get the orientation angle quarter way along the path
			var tangent = line.getTangent(this.distance);
			var angle = Math.atan2(-tangent.z, tangent.x);

			// cloud.particleCloud.rotation.y = angle;

			// move the man to that position
			cloud.particleCloud.position.set(pathPosition.x, pathPosition.y, pathPosition.z);

			cloud.particleCloud.updateMatrix();
        })
        .start();
};

THREEGAME.DefenseTower.prototype.constructor = THREE.DefenseTower;

