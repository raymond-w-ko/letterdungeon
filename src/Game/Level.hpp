#pragma once

#include "Game/Tile.hpp"

class Level {
 public:
  Level(Ogre::SceneManager* scene_manager);
  ~Level();
  
 private:
  Ogre::SceneManager* mSceneManager;
  std::vector<Tile> mTiles;
};
