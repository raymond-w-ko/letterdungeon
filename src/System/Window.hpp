#pragma once

class Game;

class Window {
 public:
  Window(Game* parent);
  ~Window();
  
  void* GetNativeWindowHandle();
  
  bool ProcessEvents();
  
 private:
  Game* mParent;
  SDL_Window* mSdlWindow;
  SDL_GameController* mGameController;
};
