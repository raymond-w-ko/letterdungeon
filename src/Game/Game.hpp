#pragma once

class Window;
class VR;
class Level;

class Game {
 public:
  Game();
  ~Game();
  
  void RecenterHmd();
  
  void Loop();
  
  void Start();
  
  void SetRotateStrength(float strength);
  
 private:
  void initOgre();
  void initOgreAssets();
  void initBodyAndHead();
  
  void rotatePlayer();
      
  std::unique_ptr<Window> mWindow;
  std::unique_ptr<Ogre::Root> mRoot;
  std::unique_ptr<VR> mVR;
  
  Ogre::RenderWindow* mRenderWindow;
  Ogre::GL3PlusRenderSystem* mRenderSystem;

  Ogre::SceneManager* mSceneManager;
  Ogre::SceneNode* mRootNode;
  Ogre::SceneNode* mBodyNode;
  Ogre::SceneNode* mHeadNode;
  Ogre::SceneNode* mHeadDisplacementNode;
  Ogre::Camera* mEyeCameras[2];
  
  std::unique_ptr<Level> mLevel;
  
  float mFrameTime;
  float mRotateStrength;
};
