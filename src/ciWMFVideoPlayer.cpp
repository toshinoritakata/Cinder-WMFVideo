#include "cinder/app/App.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/Texture.h"
#include "ciWMFVideoPlayerUtils.h"
#include "ciWMFVideoPlayer.h"

using namespace std;
using namespace ci;
using namespace ci::app;

typedef std::pair<HWND,ciWMFVideoPlayer*> PlayerItem;
list<PlayerItem> g_WMFVideoPlayers;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
// Message handlers

ciWMFVideoPlayer* findPlayers(HWND hwnd)
{
	for each (PlayerItem e in g_WMFVideoPlayers)
	{
		if (e.first == hwnd) return e.second;
	}
	return NULL;
}

int  ciWMFVideoPlayer::_instanceCount=0;

ciWMFVideoPlayer::ciWMFVideoPlayer() : _player(NULL)
{
	if (_instanceCount ==0)  {
		HRESULT hr = MFStartup(MF_VERSION);
	  if (!SUCCEEDED(hr))
		{
			//ofLog(OF_LOG_ERROR, "ciWMFVideoPlayer: Error while loading MF");
		}
	}

	_id = _instanceCount;
	_instanceCount++;
	this->InitInstance();
	
	_waitForLoadedToPlay = false;
	_sharedTextureCreated = false;	

	_waitToSetVolume = false;
	_currentVolume = 1.0f;
}
	 
ciWMFVideoPlayer::~ciWMFVideoPlayer() {
	if (_player)
    {
		_player->Shutdown();
		//if (_sharedTextureCreated) _player->m_pEVRPresenter->releaseSharedTexture();
        SafeRelease(&_player);
    }

	std::cout << "Player " << _id << " Terminated" << std::endl;
	_instanceCount--;
	if (_instanceCount == 0) 
	{
		 MFShutdown();
		 cout << "Shutting down MF" << endl;
	}
}

void ciWMFVideoPlayer::forceExit()
{
	if (_instanceCount != 0) 
	{
		cout << "Shutting down MF some ciWMFVideoPlayer remains" << endl;
		MFShutdown();
	}
}

bool ciWMFVideoPlayer::loadMovie(string name, string audioDevice)  
{
	return loadMovie(getAssetPath(name), audioDevice);
}

bool ciWMFVideoPlayer::loadMovie(ci::fs::path path, string audioDevice)
{
	 if (!_player)
	 { 
		//ofLogError("ciWMFVideoPlayer") << "Player not created. Can't open the movie.";
		 return false;
	}

	//fs::path path = getAssetPath(name);
	DWORD fileAttr = GetFileAttributesW(path.c_str());
	if (fileAttr == INVALID_FILE_ATTRIBUTES) 
	{
		stringstream s;
		s << "The video file '" << path << "'is missing.";
		//ofLog(OF_LOG_ERROR,"ciWMFVideoPlayer:" + s.str());
		return false;
	}
	
	//cout << "Videoplayer[" << _id << "] loading " << name << endl;
	HRESULT hr = S_OK;
	string s = path.string();
	std::wstring w(s.length(), L' ');
	std::copy(s.begin(), s.end(), w.begin());

	std::wstring a(audioDevice.length(), L' ');
	std::copy(audioDevice.begin(), audioDevice.end(), a.begin());

	hr = _player->OpenURL( w.c_str(), a.c_str() );
	if (!_sharedTextureCreated)
	{
		_width = _player->getWidth();
		_height = _player->getHeight();

		gl::Texture::Format format;
		format.setInternalFormat(GL_RGBA);
		format.setTargetRect();
		_tex = gl::Texture::create(_width,_height, format);
		//_tex.allocate(_width,_height,GL_RGBA,true);
		_player->m_pEVRPresenter->createSharedTexture(_width, _height, _tex->getId());
		_sharedTextureCreated = true;
	}
	else 
	{
		if ((_width != _player->getWidth()) || (_height != _player->getHeight()))
		{
			_player->m_pEVRPresenter->releaseSharedTexture();

			_width = _player->getWidth();
			_height = _player->getHeight();

			gl::Texture::Format format;
			format.setInternalFormat(GL_RGBA);
			format.setTargetRect();
			_tex = gl::Texture::create(_width,_height, format);
			//_tex.allocate(_width,_height,GL_RGBA,true);
			_player->m_pEVRPresenter->createSharedTexture(_width, _height, _tex->getId());
		}
	}

	_waitForLoadedToPlay = false;
	return true;
}

void ciWMFVideoPlayer::draw(ci::Area area, ci::Rectf rect) 
{
	_player->m_pEVRPresenter->lockSharedTexture();	
	gl::draw(_tex, area, rect);
	_player->m_pEVRPresenter->unlockSharedTexture();
} 

void ciWMFVideoPlayer::draw(ci::Rectf rect)
{
	_player->m_pEVRPresenter->lockSharedTexture();	
	gl::draw(_tex, rect);
	_player->m_pEVRPresenter->unlockSharedTexture();
}

 void ciWMFVideoPlayer::draw(int x, int y, int w, int h) 
 {
	_player->m_pEVRPresenter->lockSharedTexture();

	//_tex.draw(x,y,w,h);
	gl::draw(_tex, Rectf(x, y, x+w, y+h));

	_player->m_pEVRPresenter->unlockSharedTexture();
 }

bool  ciWMFVideoPlayer:: isPlaying() {
	return _player->GetState() == Started;
 }
bool  ciWMFVideoPlayer:: isStopped() {
	return (_player->GetState() == Stopped || _player->GetState() == Paused);
 }

bool  ciWMFVideoPlayer:: isPaused() 
{
	return _player->GetState() == Paused;
}

 void	ciWMFVideoPlayer::	close() {
	 _player->Shutdown();
	 _currentVolume = 1.0f;
	 _waitToSetVolume = false;

}
void	ciWMFVideoPlayer::	update() {
	if (!_player) return;
	if ((_waitForLoadedToPlay) && _player->GetState() == Paused)
	{
		_waitForLoadedToPlay=false;
		_player->Play();
		
	}

	if (_waitToSetVolume) {
		_player->setVolume(_currentVolume);
	}
	return;
 }

void	ciWMFVideoPlayer::	play() 
{
	if (!_player) return;
	if (_player->GetState()  == OpenPending) _waitForLoadedToPlay = true;
	_player->Play();
}

void	ciWMFVideoPlayer::	stop() 
{
	_player->Stop();
}

void	ciWMFVideoPlayer::	pause() 
{
	_player->Pause();
}

float 			ciWMFVideoPlayer::	getPosition() {
	return _player->getPosition();
}

float 			ciWMFVideoPlayer::	getDuration() {
	return _player->getDuration();
}

void ciWMFVideoPlayer::setPosition(float pos)
{
	_player->setPosition(pos);
}

void ciWMFVideoPlayer::setVolume(float vol)
{
	if ((_player) && (_player->GetState() != OpenPending) && (_player->GetState() != Closing) && (_player->GetState() != Closed)) {
		_player->setVolume(vol);
		_waitToSetVolume = false;
	}
	else {
		_waitToSetVolume = true;
	}	
	_currentVolume = vol;
}

float ciWMFVideoPlayer::getVolume()
{
	return _player->getVolume();
}


float ciWMFVideoPlayer::getHeight() { return _player->getHeight(); }
float ciWMFVideoPlayer::getWidth() { return _player->getWidth(); }
void  ciWMFVideoPlayer::setLoop(bool isLooping) { _isLooping = isLooping; _player->setLooping(isLooping); }

//-----------------------------------
// Prvate Functions
//-----------------------------------

// Handler for Media Session events.
void ciWMFVideoPlayer::OnPlayerEvent(HWND hwnd, WPARAM pUnkPtr)
{
    HRESULT hr = _player->HandleEvent(pUnkPtr);
    if (FAILED(hr))
    {
        //ofLogError("ciWMFVideoPlayer", "An error occurred.");
    }
 }

LRESULT CALLBACK WndProcDummy(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
		case WM_CREATE:
		{
			return DefWindowProc(hwnd, message, wParam, lParam);
		}
		default:
		{
			ciWMFVideoPlayer*   myPlayer = findPlayers(hwnd);
			if (!myPlayer) 
				return DefWindowProc(hwnd, message, wParam, lParam);
			return myPlayer->WndProc (hwnd, message, wParam, lParam);
		}
    }
    return 0;
}

LRESULT  ciWMFVideoPlayer::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_APP_PLAYER_EVENT:
			OnPlayerEvent(hwnd, wParam);
			break;

		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}

//  Create the application window.
BOOL ciWMFVideoPlayer::InitInstance()
{
	PCWSTR szWindowClass = L"MFBASICPLAYBACK" ;
    HWND hwnd;
    WNDCLASSEX wcex;

	//   g_hInstance = hInst; // Store the instance handle.
    // Register the window class.
    ZeroMemory(&wcex, sizeof(WNDCLASSEX));
    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW  ;

    wcex.lpfnWndProc    =  WndProcDummy;
	//  wcex.hInstance      = hInst;
	wcex.hbrBackground  = (HBRUSH)(BLACK_BRUSH);
    // wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_MFPLAYBACK);
    wcex.lpszClassName  = szWindowClass;

    if (RegisterClassEx(&wcex) == 0)
    {
       // return FALSE;
    }

    // Create the application window.
    hwnd = CreateWindow(szWindowClass, L"", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, NULL, NULL);
    if (hwnd == 0)
    {
        return FALSE;
    }

	g_WMFVideoPlayers.push_back(std::pair<HWND,ciWMFVideoPlayer*>(hwnd,this));
	HRESULT hr = CPlayer::CreateInstance(hwnd, hwnd, &_player); 

	LONG style2 = ::GetWindowLong(hwnd, GWL_STYLE);  
    style2 &= ~WS_DLGFRAME;
    style2 &= ~WS_CAPTION; 
    style2 &= ~WS_BORDER; 
    style2 &= WS_POPUP;
    LONG exstyle2 = ::GetWindowLong(hwnd, GWL_EXSTYLE);  
    exstyle2 &= ~WS_EX_DLGMODALFRAME;  
    ::SetWindowLong(hwnd, GWL_STYLE, style2);  
    ::SetWindowLong(hwnd, GWL_EXSTYLE, exstyle2);  

	_hwndPlayer = hwnd;
    UpdateWindow(hwnd);
	
    return TRUE;
}

