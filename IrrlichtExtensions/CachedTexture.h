#ifndef CACHEDTEXTURE_H_INCLUDED
#define CACHEDTEXTURE_H_INCLUDED

#include <ITexture.h>
#include <IVideoDriver.h>

//! Encapsulation for Textures to use them in the LRUCache
class CachedTexture{

	public:
	irr::video::ITexture* tex;
	irr::video::IVideoDriver* driver;

	CachedTexture(irr::video::ITexture* Tex, irr::video::IVideoDriver* Driver):tex(Tex), driver(Driver){};

	~CachedTexture(){
		if(tex){driver->removeTexture(tex);}
	}	

};

#endif
