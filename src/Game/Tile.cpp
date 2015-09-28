#include "PrecompiledHeaders.hpp"

#include "Game/Tile.hpp"

Tile::Tile(Ogre::SceneManager* scene_manager, int x, int y,
           std::mt19937& gen)
    : x(x),
      y(y)
{
  auto material_man = Ogre::MaterialManager::getSingletonPtr();
  
  auto root = scene_manager->getRootSceneNode(Ogre::SCENE_STATIC);
  mSceneNode = root->createChildSceneNode(Ogre::SCENE_STATIC);
  mSceneNode->setPosition(x + 0.5f, 0, y + 0.5f);

  mEntity = scene_manager->createEntity(
      "LevelTile", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
      Ogre::SCENE_STATIC);
  
  auto base = material_man->getByName("OneColor");
  std::string material_name =
      "Tile"
      + boost::lexical_cast<std::string>(x) + ","
      + boost::lexical_cast<std::string>(y);
  auto material = base->clone(material_name);
  auto pass = material->getTechnique(0)->getPass(0);
  auto params = pass->getFragmentProgramParameters();
  std::uniform_real_distribution<float> dis(0.0f, 1.0f);
  params->setNamedConstant(
      "Color",
      Ogre::Vector4(dis(gen), dis(gen), dis(gen), 1.0f));
  
  mEntity->setMaterial(material);
  mSceneNode->attachObject(mEntity);
}

Tile::~Tile() {
  ;
}
