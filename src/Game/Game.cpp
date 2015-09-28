#include "PrecompiledHeaders.hpp"
#include "Game/Game.hpp"

#include "System/Window.hpp"
#include "System/VR.hpp"

#include "Game/Level.hpp"

Game::Game()
    : mRenderWindow(nullptr),
      mRenderSystem(nullptr),
      mSceneManager(nullptr),
      mRootNode(nullptr),
      mBodyNode(nullptr),
      mHeadNode(nullptr),
      mHeadDisplacementNode(nullptr),
      mEyeCameras{nullptr, nullptr},
      mFrameTime(0.0),
      mRotateStrength(0.0)
{
  mWindow = std::make_unique<Window>(this);
  
  this->initOgre();
  this->initOgreAssets();
  
  mVR = std::make_unique<VR>();
  this->initBodyAndHead();
  
  mVR->CreateEyeRenderTargets();
  mVR->SetupCompositor(mRoot.get(), mSceneManager, mEyeCameras);
  mVR->SetupMirroring(mRoot.get(), mRenderWindow);
}

void Game::initOgre() {
  // create root
  std::string plugin_filename = "";
  std::string config_filename = "";
  std::string log_filename = "Ogre.log";
  mRoot = std::make_unique<Ogre::Root>(
      plugin_filename, config_filename, log_filename);

  mRoot->installPlugin(new Ogre::GL3PlusPlugin());
  mRoot->installPlugin(new Ogre::ParticleFXPlugin());

  // create renderer
  auto rendersystems = mRoot->getAvailableRenderers();
  mRenderSystem = static_cast<Ogre::GL3PlusRenderSystem*>(rendersystems[0]);
  // never use this, causes horrible stuttering in VR
  mRenderSystem->setFixedPipelineEnabled(false);
  mRoot->setRenderSystem(mRenderSystem);

  static const bool autocreate_window = false;
  mRoot->initialise(autocreate_window);

  // never set vsync to true as the Oculus SDK takes care of that with magic
  Ogre::NameValuePairList params;
  params["vsync"] = "vsync";
  params["gamma"] = "true";
  params["externalWindowHandle"] = Ogre::StringConverter::toString(
      (size_t)mWindow->GetNativeWindowHandle());
  mRenderWindow = mRoot->createRenderWindow(
      "OGRE Render Window",
      1920 / 2, 1080 / 2,
      false,
      &params);
  mRenderWindow->setActive(true);
  mRenderWindow->setVisible(true);
  mRenderWindow->setVSyncEnabled(false);
  
  static const int num_threads = 2;
  static const auto culling_method = Ogre::INSTANCING_CULLING_SINGLETHREAD;
  mSceneManager = mRoot->createSceneManager(
      "DefaultSceneManager", num_threads, culling_method);
}

void Game::initOgreAssets() {
  using namespace Ogre;
  
  // Load resource paths from config file
  ConfigFile config_file;
  config_file.load("resources.cfg");

  // Go through all sections & settings in the file
  ConfigFile::SectionIterator section_iter = config_file.getSectionIterator();

  String group_name, location_type, location_name;
  while (section_iter.hasMoreElements()) {
    group_name = section_iter.peekNextKey();
    ConfigFile::SettingsMultiMap* settings = section_iter.getNext();
    for (auto i = settings->begin(); i != settings->end(); ++i) {
      location_type = i->first;
      location_name = i->second;
      static const bool recursive = false;
      ResourceGroupManager::getSingleton().addResourceLocation(
          location_name, location_type, group_name, recursive);
    }
  }

  ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
}

void Game::initBodyAndHead() {
  mRootNode = mSceneManager->getRootSceneNode();
  
  // for gross movement by WASD and control sticks
  mBodyNode = mRootNode->createChildSceneNode();
  mBodyNode->setPosition(Ogre::Vector3(2.5f, 0, 2.5f));
  mBodyNode->setDirection(-Ogre::Vector3::UNIT_Z);
  // for finer movement by camera positional tracking
  mHeadNode = mBodyNode->createChildSceneNode();
  mHeadNode->setInheritOrientation(true);
  mHeadNode->setPosition(Ogre::Vector3(0, mVR->GetEyeHeight(), 0));
  mHeadDisplacementNode = mHeadNode->createChildSceneNode();
  mHeadDisplacementNode->setInheritOrientation(true);
  
  Ogre::Camera* left_eye = mSceneManager->createCamera("LeftEye");
  left_eye->detachFromParent();
  mHeadDisplacementNode->attachObject(left_eye);
  mEyeCameras[ovrEye_Left] = left_eye;

  Ogre::Camera* right_eye = mSceneManager->createCamera("RightEye");
  right_eye->detachFromParent();
  mHeadDisplacementNode->attachObject(right_eye);
  mEyeCameras[ovrEye_Right] = right_eye;
}

Game::~Game() {
  ;
}

void Game::RecenterHmd() {
  mVR->RecenterHmd();
}

void Game::Loop() {
  using std::chrono::high_resolution_clock;
  using std::chrono::duration_cast;
  
  auto last_frame_start = high_resolution_clock::now();
  while (true) {
    auto now = high_resolution_clock::now();
    auto last_frame_duration = duration_cast<std::chrono::nanoseconds>(
        now - last_frame_start);
    last_frame_start = now;
    mFrameTime = last_frame_duration.count() / (float)1e9;
    
    if (!mWindow->ProcessEvents())
      break;

    this->rotatePlayer();
    mVR->Render(mHeadDisplacementNode, mEyeCameras, mRoot.get());
  }
}

void Game::Start() {
  mLevel = std::make_unique<Level>(mSceneManager);
}

void Game::rotatePlayer() {
  auto amount = Ogre::Radian(mRotateStrength * mFrameTime * Ogre::Math::PI);
  mBodyNode->yaw(amount);
}

void Game::SetRotateStrength(float strength) {
  strength = boost::algorithm::clamp(strength, -1.0f, 1.0f);
  // dead zone to prevent unlimited rotation works
  if (std::fabs(strength) <= 0.15) {
    strength = 0.0f;
  }
  mRotateStrength = strength;
}
