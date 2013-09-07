
// LOAD_MANAGER

var THREEGAME = THREEGAME || {
    CHARACTERS: [],
};

THREEGAME.DefenseTower = function( x, y, z, options ) {

    THREE.Object3D.apply( this );

    var self = this,

        x = x ||  0,
        y = y || 28,
        z = z ||  1,
        options = options || {}
        ;

    this.bulletSpeed = options.bulletSpeed || 10;
    this.fireSpeed = options.fireSpeed || 1;


    /////

    this._firing = false;
    this._currentTweens = [];

    this.options = _.merge({
                start: true,
        fireIntensity: 20000,
            transform: function( obj3d ){ return obj3d },
           orbTexture: options.texture || THREE.ImageUtils.loadTexture( "textures/lensflare/lensflare1_alpha.png" ),
          fireTexture: options.texture || THREE.ImageUtils.loadTexture( "textures/lensflare/lensflare0_alpha.png" ),
    } , options );

    // self.fireCloud = new Extras.ParticleCloud( 10000, self.options.fireTexture );

    var loader = new THREE.ColladaLoader();
    loader.load( 'obj/dota/lantern.dae', function ( loaded ) {

        options.transform( loaded );

        self.aura = Extras.Aura( 'point', self.options.fireIntensity, self.options.orbTexture, pointLight );
        self.aura.particleCloud.position.set( x, y, z );
        self.add( self.aura.particleCloud );
        
        var lantern = loaded.scene.children[ 0 ];
        delete loaded;

        self.add( lantern );

        var selfUpdate = _.bind( self.update, self );

        if ( self.options.start ) {
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

    var charDistance;
    _.each( THREEGAME.CHARACTERS, function( c ) {
        charDistance = c.position.distanceTo( self.position );
        if ( charDistance < 70 ) {

            // this.fireSpeed += (70 - charDistance);

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
    
    var startPosition = this.position.clone().setY( 28 );
    var vectorPosition = target.position.clone().add(startPosition).divideScalar(2).setY( 28 + 5 );

    var self = this,
        line = new THREE.SplineCurve3([ startPosition, vectorPosition, target.position ]),
        cloud = new Extras.ParticleCloud( 10000, self.options.fireTexture, null, {
            colorHSL: .5
        }),
        cloudUpdate = _.bind( function(event){ cloud.update(event.detail.delta); }, cloud )
        ;

    var tween = new TWEEN.Tween({ distance: 0 })

        .to( { distance: 1 }, line.getLength() * self.bulletSpeed ) // use 

        .easing(TWEEN.Easing.Linear.None)

        .onStart(function(){
            container.addEventListener( 'render-update', cloudUpdate );

            self.add( cloud.particleCloud );
            cloud.start();

            setTimeout( function(){
                self._firing = false;

            }, 4000 / self.fireSpeed );


            setTimeout( function(){
                if (tween) { tween.stop(); }
                container.removeEventListener( 'render-update', cloudUpdate );

                // self._firing = false;
                self.remove( cloud.particleCloud );
            }, 1000 );
        })
        
        .onComplete(function(){
            container.removeEventListener( 'render-update', cloudUpdate );

            self.remove( cloud.particleCloud );
            cloud.stop();
            // self._firing = false;

            delete cloud;
        })
        
        .onUpdate(function(){
            // get the position data half way along the path
            var pathPosition = line.getPoint(this.distance);

            // get the orientation angle quarter way along the path
            // var tangent = line.getTangent(this.distance);
            // var angle = Math.atan2(-tangent.z, tangent.x);
            // cloud.particleCloud.rotation.y = angle;

            // random
            // pathPosition.x += (Math.random()*2 -1) * .0001;
            // pathPosition.y += (Math.random()*2 -1) * .0001;
            // pathPosition.z += (Math.random()*2 -1) * .0001;

            // move the man to that position
            cloud.particleCloud.position.set(pathPosition.x, pathPosition.y, pathPosition.z);

            cloud.particleCloud.updateMatrix();
        })
        .start();
};

THREEGAME.DefenseTower.prototype.constructor = THREE.DefenseTower;

