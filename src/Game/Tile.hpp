#pragma once

class Tile {
 public:
  Tile(Ogre::SceneManager* scene_manager, int x, int y,
       std::mt19937& gen);
  ~Tile();
  
 private:
  int x, y;
  
  Ogre::SceneNode* mSceneNode;
  Ogre::Entity* mEntity;
};
