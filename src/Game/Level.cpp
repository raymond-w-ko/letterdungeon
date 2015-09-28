#include "PrecompiledHeaders.hpp"

#include "Game/Level.hpp"

Level::Level(Ogre::SceneManager* scene_manager)
    : mSceneManager(scene_manager)
{
  auto mesh_man = Ogre::MeshManager::getSingletonPtr();
  Ogre::Plane plane(Ogre::Vector3::UNIT_Y,
                    Ogre::Vector3(0.0f, 0.0f, 0.0f));
  mesh_man->createPlane(
      "LevelTile",
      Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
      plane, 1.0f, 1.0f,
      1, 1,
      true, 1,
      1.0f, 1.0f,
      Ogre::Vector3::UNIT_Z);
  
  std::random_device rd;
  std::mt19937 gen(rd());
  
  for (int x = 0; x < 8; ++x) {
    for (int y = 0; y < 8; ++y) {
      mTiles.emplace_back(mSceneManager, x, y, gen);
    }
  }
  
  auto cube = scene_manager->createEntity(
      Ogre::SceneManager::PT_CUBE,
      Ogre::SCENE_STATIC);
  auto root = scene_manager->getRootSceneNode(Ogre::SCENE_STATIC);
  auto node = root->createChildSceneNode(Ogre::SCENE_STATIC);
  node->attachObject(cube);
  node->setPosition(1.5f, 1.0f, 1.5f);
  float scale = 1 / 100.0f;
  node->setScale(Ogre::Vector3(scale, scale, scale));
  auto material_man = Ogre::MaterialManager::getSingletonPtr();
  auto material = material_man->getByName("OneColor");
  cube->setMaterial(material);
}

Level::~Level() {
  auto mesh_man = Ogre::MeshManager::getSingletonPtr();
  mesh_man->remove("LevelTile");
}
