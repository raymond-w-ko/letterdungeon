#include "PrecompiledHeaders.hpp"

#include "System/Window.hpp"
#include "Game/Game.hpp"

Window::Window(Game* parent)
    : mParent(parent),
      mGameController(nullptr)
{
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
  
  static const unsigned int flags = SDL_WINDOW_SHOWN;
  int x = SDL_WINDOWPOS_CENTERED;
  int y = SDL_WINDOWPOS_CENTERED;
  mSdlWindow = SDL_CreateWindow(
      "Letter Dungeon",
      x, y,
      1600, 900,
      flags);
  if (!mSdlWindow) {
    throw std::runtime_error("Could not create SDL2 window!");
  }
  
  mGameController = SDL_GameControllerOpen(0);
}

Window::~Window() {
  if (mGameController) {
    SDL_GameControllerClose(mGameController);
  }
  SDL_DestroyWindow(mSdlWindow);
  SDL_Quit();
}

void* Window::GetNativeWindowHandle() {
  SDL_SysWMinfo window_info;
  SDL_VERSION(&window_info.version);
  SDL_GetWindowWMInfo(mSdlWindow, &window_info);
#ifdef _WIN32
  return (void*) window_info.info.win.window;
#else
  throw std::runtime_error(
      "GetNativeWindowHandle() not implemented for this OS");
  return nullptr;
#endif
}

bool Window::ProcessEvents() {
  SDL_Event e;
  
  float render_quality = 1.0f;
  float fov = 1.0f;
  bool render_quality_dirty = false;

  while (SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_QUIT:
        return false;
      case SDL_KEYDOWN: {
        switch (e.key.keysym.scancode) {
          case SDL_SCANCODE_SPACE: {
            mParent->RecenterHmd();
            break;
          }
          case SDL_SCANCODE_W: {
            render_quality += 0.1f;
            render_quality_dirty = true;
            break;
          }
          case SDL_SCANCODE_S: {
            render_quality -= 0.1f;
            render_quality_dirty = true;
            break;
          }
          case SDL_SCANCODE_A: {
            fov -= 0.1f;
            render_quality_dirty = true;
            break;
          }
          case SDL_SCANCODE_D: {
            fov += 0.1f;
            render_quality_dirty = true;
            break;
          }
        }
        break;
      }
      case SDL_CONTROLLERBUTTONDOWN: {
        int button = e.cbutton.button;
        std::string strbutton = boost::lexical_cast<std::string>(button);
        OutputDebugStringA((strbutton + "\n").c_str());
        
        switch (button) {
          case 5: {
            mParent->RecenterHmd();
            break;
          }
        }
        break;
      }
      case SDL_CONTROLLERAXISMOTION: {
        int axis = e.caxis.axis;
        int value = e.caxis.value;
        /* std::stringstream ss; */
        /* ss << axis << " " << value << "\n"; */
        /* OutputDebugStringA(ss.str().c_str()); */
        switch (axis) {
          case 2: {
            // camera rotate along human up down axis
            float strength = (float)value / (0xFFFF / 2);
            // this feels natural to me, but some players prefer inverted, so
            // this will have to be a setting
            strength = -strength;
            mParent->SetRotateStrength(strength);
            break;
          }
        }
        break;
      }
    }
  }
  
  return true;
}
