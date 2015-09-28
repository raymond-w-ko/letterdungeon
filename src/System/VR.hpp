#pragma once

class VR {
 public:
  struct Eye {
    // 1.0 gets you a render target size that gives you 1.0 center pixels after
    // distortion. Decrease this if you need more performance.
    float RenderQuality;
    // 1.0 is the default FOV. Decrease this as a last resort if you need more
    // performance, as this will severely break immersion by reduce the field of view
    float FovQuality;
    
    ovrFovPort Fov;
    ovrSizei TextureSize;
    ovrEyeRenderDesc RenderDesc;
    
    ovrSwapTextureSet* SwapTextureSet;
    struct Textures_ {
      Ogre::TexturePtr Texture;
      Ogre::RenderTarget* RenderTarget;
    } Textures[2];
    
    Ogre::CompositorWorkspace* CompositorWorkspaces[2];
  };
  
  static void onOculusSDKLogMessage(uintptr_t userData,
                                    int level, const char* message);
  static void VR::ShimInOculusTexture(Ogre::Texture* _ogre_texture,
                                      ovrTexture* _ovr_texture);
  static void VR::DeleteShimmedOgreTexture(Ogre::TexturePtr& texture_ptr);
  
  VR();
  ~VR();
  
  void onOculusSDKLogMessage(int level, const char* message);
  
  void CreateEyeRenderTargets(float render_quality = 1.0f,
                              float fov_quality = 1.0f);
  void SetupCompositor(
      Ogre::Root* root,
      Ogre::SceneManager* scene_manager,
      Ogre::Camera** cameras);
  void SetupMirroring(Ogre::Root* root,
                      Ogre::RenderWindow* render_window);
  
  float GetPlayerHeight() const { return mPlayerHeight; }
  // scale down since most people are sitting
  float GetEyeHeight() const { return mEyeHeight; }
  
  void Render(Ogre::SceneNode* head_displacement_node,
              Ogre::Camera** cameras,
              Ogre::Root* root);
  
  void RecenterHmd();

 private:
  int mWorkspaceRenderOrder;
  
  ovrHmd mHmd;
  ovrHmdDesc mHmdDesc;
  float mPlayerHeight;
  float mEyeHeight;
  // not used as of 0.7.0 SDK
  ovrGraphicsLuid mGraphicsLuid;
  
  Eye mEyes[ovrEye_Count];
  
  ovrTexture* mOvrMirrorTexture;
  Ogre::TexturePtr mMirrorTexture;
  Ogre::SceneManager* mMirrorSceneManager;
  Ogre::CompositorWorkspace* mMirrorWorkspace;
};
