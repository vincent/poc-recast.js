
THREEGAME.HUD = THREEGAME.HUD || {};

THREEGAME.HUD.sidemenu = function () {
	
	this.root = document.getElementById('hud');

	this.root.classList.add( 'animated' );
};

THREEGAME.HUD.sidemenu.prototype.open = function () {

	this.root.classList.remove( 'fadeOutLeft' );
	//this.root.classList.add( 'fadeInLeft' );
	this.root.style.zIndex = 10000;
};

THREEGAME.HUD.sidemenu.prototype.close = function () {

	this.root.classList.remove( 'fadeInLeft' );
	//this.root.classList.add( 'fadeOutLeft' );
	this.root.style.zIndex = -2;
};

THREEGAME.HUD.sidemenu.prototype.isOpen = function () {

	return this.root.style.zIndex > 0;
	return this.root.classList.contains( 'fadeInLeft' );
};

