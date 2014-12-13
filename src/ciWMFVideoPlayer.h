#pragma once

/*
#ifdef __gl_h_
#undef __gl_h_
#endif

#ifdef __glext_h_
#undef __glext_h_
#endif

#ifdef __wglext_h_
#undef __wglext_h_
#endif

#ifdef __glxext_h_
#undef __glxext_h_
#endif*/

#include "ciWMFVideoPlayerUtils.h"
#include "presenter/EVRPresenter.h"
#include <boost/shared_ptr.hpp>

class ciWMFVideoPlayer;
class CPlayer;

namespace cinder
{
	namespace gl
	{
		class Texture;
		typedef std::shared_ptr<Texture> TextureRef;
	}
}

class ciWMFVideoPlayer {

	private:
		static int  _instanceCount;
		HWND		_hwndPlayer;
		BOOL bRepaintClient;
		
		int _width;
		int _height;

		bool _waitForLoadedToPlay;
		bool _isLooping;
		bool _waitToSetVolume;
		float _currentVolume;

		bool _sharedTextureCreated;
		
		cinder::gl::TextureRef _tex;
	
		BOOL InitInstance();
		void                OnPlayerEvent(HWND hwnd, WPARAM pUnkPtr);

	public:
	CPlayer*	_player;

	int _id;
	
	ciWMFVideoPlayer();
	 ~ciWMFVideoPlayer();

	 bool				loadMovie(std::string name, std::string audioDevice="");
	 bool				loadMovie(ci::fs::path path, std::string audioDevice="");
	 void				close();
	 void				update();
	
	 void				play();
	 void				stop();		
	 void				pause();

	 float				getPosition();
	 float				getDuration();

	 void				setPosition(float pos);

	 void				setVolume(float vol);
	 float 				getVolume();

	 float				getHeight();
	 float				getWidth();

	 bool				isPlaying(); 
	 bool				isStopped();
	 bool				isPaused();

	 void				setLoop(bool isLooping);
	 bool				isLooping() { return _isLooping; }

	void draw(int x, int y , int w, int h);
	void draw(int x, int y) { draw(x, y, static_cast<int>(getWidth()), static_cast<int>(getHeight())); }
	void draw(ci::Rectf rect);
	void draw(ci::Area area, ci::Rectf rect);

	HWND getHandle() { return _hwndPlayer;}
	LRESULT WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	static void forceExit();
};