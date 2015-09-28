#include "PrecompiledHeaders.hpp"
#include "System/VR.hpp"

VR::VR() 
    : mWorkspaceRenderOrder(0),
      mPlayerHeight(0.0f),
      mEyeHeight(0.0f),
      mOvrMirrorTexture(nullptr),
      mMirrorSceneManager(nullptr),
      mMirrorWorkspace(nullptr)
{
  memset(&mHmd, 0, sizeof(mHmd));
  memset(&mGraphicsLuid, 0, sizeof(mGraphicsLuid));
  memset(&mHmdDesc, 0, sizeof(mHmdDesc));
  mEyes[ovrEye_Left].SwapTextureSet = nullptr;
  mEyes[ovrEye_Right].SwapTextureSet = nullptr;

  ovrInitParams init_params;

  init_params.Flags = 0;
  /// When a debug library is requested, a slower debugging version of the
  //library will run which can be used to help solve problems in the
  //library and debug application code.
  /* init_params.Flags |= ovrInit_Debug; */
  /* init_params.Flags |= ovrInit_ServerOptional; */
  /* init_params.Flags |= ovrInit_RequestVersion; */
  init_params.RequestedMinorVersion = 0;
  init_params.LogCallback = VR::onOculusSDKLogMessage;
  init_params.UserData = (decltype(init_params.UserData))this;
  // 0 is default timeout
  init_params.ConnectionTimeoutMS = 0;
  if (OVR_FAILURE(ovr_Initialize(&init_params))) {
    throw std::runtime_error("failed to initialize OVR");
  }
  
  if (OVR_FAILURE(ovr_Create(&mHmd, &mGraphicsLuid))) {
    throw std::runtime_error("failed to create real or debug HMD");
  }
  mHmdDesc = ovr_GetHmdDesc(mHmd);
  
  mPlayerHeight = ovr_GetFloat(mHmd, OVR_KEY_PLAYER_HEIGHT,
                               OVR_DEFAULT_PLAYER_HEIGHT);
  mEyeHeight = ovr_GetFloat(mHmd, OVR_KEY_EYE_HEIGHT,
                            OVR_DEFAULT_EYE_HEIGHT);

  static const unsigned int supported_tracking_caps =
      ovrTrackingCap_Orientation |
      ovrTrackingCap_MagYawCorrection |
      ovrTrackingCap_Position |
      0;
  static const unsigned int required_tracking_caps = 0;
  if (OVR_FAILURE(ovr_ConfigureTracking(mHmd,
                                        supported_tracking_caps,
                                        required_tracking_caps))) {
    throw std::runtime_error("failed to configure tracking capabilities");
  }
}

VR::~VR() {
  if (mOvrMirrorTexture) {
    ovr_DestroyMirrorTexture(mHmd, mOvrMirrorTexture);
  }
  DeleteShimmedOgreTexture(mMirrorTexture);

  for (int i = 0; i < 2; ++i) {
    ovr_DestroySwapTextureSet(mHmd, mEyes[i].SwapTextureSet);
  }
  for (int i = 0; i < 2; ++i) {
    DeleteShimmedOgreTexture(mEyes[0].Textures[i].Texture);
    mEyes[0].Textures[i].RenderTarget = nullptr;

    DeleteShimmedOgreTexture(mEyes[1].Textures[i].Texture);
    mEyes[1].Textures[i].RenderTarget = nullptr;
  }
}

void VR::onOculusSDKLogMessage(uintptr_t userData,
                               int level, const char* message) {
  ((VR*)userData)->onOculusSDKLogMessage(level, message);
}
void VR::onOculusSDKLogMessage(int level, const char* message) {
  // TODO: redirect this to a file somwhere
  OutputDebugStringA(message);
}

namespace {
struct GLTextureHacker : public Ogre::GL3PlusTexture {
  void _freeInternalResourcesImpl() {
    this->freeInternalResourcesImpl();
  }
  void __createSurfaceList() {
    this->_createSurfaceList();
  }
};
} // namespace

void VR::ShimInOculusTexture(Ogre::Texture* _ogre_texture,
                                ovrTexture* _ovr_texture) {
  const unsigned char * gl_version = glGetString(GL_VERSION);
  int error;

  auto ogre_texture = static_cast<GLTextureHacker*>(_ogre_texture);
  ogre_texture->_freeInternalResourcesImpl();
  glDeleteTextures(1, &ogre_texture->mTextureID);

  ovrGLTexture* ovr_texture = reinterpret_cast<ovrGLTexture*>(_ovr_texture);
  auto texture_id = ovr_texture->OGL.TexId;
  ogre_texture->mTextureID = texture_id;
  auto texture_target = ogre_texture->getGL3PlusTextureTarget();
  auto& gl_support = ogre_texture->mGLSupport;
  glBindTexture(texture_target, texture_id);
  error = glGetError();
  // OculusRoomTiny only uses 1 level of mipmap, does not touch this
  /* static const int num_mipmaps = 0; */
  /* state_man->setTexParameteri( */
  /*     texture_target, GL_TEXTURE_MAX_LEVEL, num_mipmaps); */
  /* error = glGetError(); */
  // copied from OculusRoomTiny demo
  glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  error = glGetError();
  glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  error = glGetError();
  glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  error = glGetError();
  glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  error = glGetError();
  // only for textures not displayed on HMD
  /* glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, */
  /*              ogre_texture->getWidth(), ogre_texture->getHeight(), 0, */
  /*              GL_RGBA, GL_UNSIGNED_BYTE, 0); */
  /* error = glGetError(); */
  ogre_texture->__createSurfaceList();
}

void VR::DeleteShimmedOgreTexture(Ogre::TexturePtr& texture_ptr) {
  if (!texture_ptr.get())
    return;
  auto texture = static_cast<Ogre::GL3PlusTexture*>(texture_ptr.get());
  // HACK BEGIN
  texture->mSurfaceList.clear();
  texture->mTextureID = 0;
  // HACK END
  std::string texture_name(texture_ptr->getName());
  texture_ptr.setNull();
  Ogre::TextureManager::getSingleton().remove(texture_name);
}

void VR::CreateEyeRenderTargets(float render_quality, float fov_quality) {
  using namespace boost::algorithm;
  
  ovrResult ret;
  render_quality = clamp(render_quality, 0.05f, 2.0f);
  fov_quality = clamp(fov_quality, 0.05f, 1.0f);

  for (int i = 0; i < ovrEye_Count; ++i) {
    auto& eye = mEyes[i];
    ovrEyeType eye_type = (ovrEyeType)i;
    eye.RenderQuality = render_quality;
    eye.FovQuality = fov_quality;

    auto& fov = mEyes[i].Fov;
    const auto& max_fov = mHmdDesc.MaxEyeFov[i];
    const auto& default_fov = mHmdDesc.DefaultEyeFov[i];
    fov.UpTan = std::min(eye.FovQuality * default_fov.UpTan, max_fov.UpTan);
    fov.DownTan = std::min(eye.FovQuality * default_fov.DownTan,
                           max_fov.DownTan);
    fov.LeftTan = std::min(eye.FovQuality * default_fov.LeftTan,
                           max_fov.LeftTan);
    fov.RightTan = std::min(eye.FovQuality * default_fov.RightTan,
                            max_fov.RightTan);

    eye.TextureSize = ovr_GetFovTextureSize(mHmd, eye_type,
                                            fov, eye.RenderQuality);
    eye.RenderDesc = ovr_GetRenderDesc(mHmd, eye_type, eye.Fov);

    if (eye.SwapTextureSet) {
      ovr_DestroySwapTextureSet(mHmd, eye.SwapTextureSet);
      eye.SwapTextureSet = nullptr;
    }
    ret = ovr_CreateSwapTextureSetGL(
        mHmd, GL_SRGB8_ALPHA8, eye.TextureSize.w, eye.TextureSize.h,
        &eye.SwapTextureSet);
    if (OVR_FAILURE(ret)) {
      throw std::runtime_error("failed to create Oculus SDK texture set");
    }

    // create Ogre RenderTextures by shimming in the Oculus supplied textures
    for (int texture_index = 0;
         texture_index < eye.SwapTextureSet->TextureCount;
         ++texture_index) {
      auto& slot = eye.Textures[texture_index];
      // set the OGRE texture's gut to OpenGL invalid 0 texture so we don't do
      // a double free.
      if (slot.Texture.get()) {
        DeleteShimmedOgreTexture(slot.Texture);
      }

      static const int num_mipmaps = 0;
      // MUST BE 0, or mysterious NULL pointer exception, also implied on
      // OculusRoomTiny demo
      static const int multisample = 0;
      // must be true for OGRE to create GL_SRGB8_ALPHA8
      // source: if (hwGamma) in OgreGLPixelFormat.cpp
      static const bool hw_gamma_correction = true;
      // this also must be set to this format for OGRE to create GL_SRGB8_ALPHA8
      static const Ogre::PixelFormat format = Ogre::PF_B8G8R8A8;
      std::string name =
          "EyeRenderTarget"
          + boost::lexical_cast<std::string>(i)
          + "SwapTexture"
          + boost::lexical_cast<std::string>(texture_index);
      slot.Texture = Ogre::TextureManager::getSingleton().createManual(
          name,
          Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
          Ogre::TEX_TYPE_2D,
          eye.TextureSize.w, eye.TextureSize.h,
          num_mipmaps,
          format,
          Ogre::TU_RENDERTARGET,
          nullptr,
          hw_gamma_correction,
          multisample);

      ShimInOculusTexture(slot.Texture.get(),
                          &eye.SwapTextureSet->Textures[texture_index]);
      slot.RenderTarget = slot.Texture->getBuffer()->getRenderTarget();
    }
  }
}


void VR::SetupCompositor(
      Ogre::Root* root,
      Ogre::SceneManager* scene_manager,
      Ogre::Camera** cameras)
{
  auto compositor_manager = root->getCompositorManager2();
  
  compositor_manager->removeAllWorkspaces();

  auto red = Ogre::ColourValue(1.0f, 0.0f, 0.0f, 1.0f);
  auto blue = Ogre::ColourValue(0.0f, 0.0f, 1.0f, 1.0f);
  auto black = Ogre::ColourValue(0.0f, 0.0f, 0.0f, 1.0f);
  if (!compositor_manager->hasWorkspaceDefinition("SwapSet0")) {
    compositor_manager->createBasicWorkspaceDef(
        "SwapSet0", black);
  }
  if (!compositor_manager->hasWorkspaceDefinition("SwapSet1")) {
    compositor_manager->createBasicWorkspaceDef(
        "SwapSet1", black);
  }

  int swap_number;
  Ogre::CompositorWorkspace* workspace;

  swap_number = 0;
  // left eye, swap texture 0, left camera
  workspace = compositor_manager->addWorkspace(
      scene_manager,
      mEyes[ovrEye_Left].Textures[swap_number].RenderTarget,
      cameras[ovrEye_Left],
      "SwapSet0", false, mWorkspaceRenderOrder++);
  mEyes[ovrEye_Left].CompositorWorkspaces[swap_number] = workspace;
  // right eye, swap texture 0, right camera
  workspace = compositor_manager->addWorkspace(
      scene_manager,
      mEyes[ovrEye_Right].Textures[swap_number].RenderTarget,
      cameras[ovrEye_Right],
      "SwapSet0", false, mWorkspaceRenderOrder++);
  mEyes[ovrEye_Right].CompositorWorkspaces[swap_number] = workspace;

  swap_number = 1;
  // left eye, swap texture 1, left camera
  workspace = compositor_manager->addWorkspace(
      scene_manager,
      mEyes[ovrEye_Left].Textures[swap_number].RenderTarget,
      cameras[ovrEye_Left],
      "SwapSet1", false, mWorkspaceRenderOrder++);
  mEyes[ovrEye_Left].CompositorWorkspaces[swap_number] = workspace;
  // right eye, swap texture 1, right camera
  workspace = compositor_manager->addWorkspace(
      scene_manager,
      mEyes[ovrEye_Right].Textures[swap_number].RenderTarget,
      cameras[ovrEye_Right],
      "SwapSet1", false, mWorkspaceRenderOrder++);
  mEyes[ovrEye_Right].CompositorWorkspaces[swap_number] = workspace;
}

void VR::SetupMirroring(Ogre::Root* root,
                        Ogre::RenderWindow* render_window) {
  using namespace Ogre;
  
   // should match SDL created window size
  int w = 1600;
  int h = 900;

  auto ret = ovr_CreateMirrorTextureGL(
      mHmd, GL_SRGB8_ALPHA8, w, h, &mOvrMirrorTexture);
  if (OVR_FAILURE(ret)) {
    throw std::runtime_error("Oculus SDK failed to create mirror texture");
  }

  static const int num_mipmaps = 0;
  // MUST BE 0, or mysterious NULL pointer exception, also implied on
  // OculusRoomTiny demo
  static const int multisample = 0;
  // must be true for OGRE to create GL_SRGB8_ALPHA8
  // source: if (hwGamma) in OgreGLPixelFormat.cpp
  static const bool hw_gamma_correction = true;
  // this also must be set to this format for OGRE to create GL_SRGB8_ALPHA8
  static const Ogre::PixelFormat format = Ogre::PF_B8G8R8A8;
  std::string name = "OculusMirror";
  mMirrorTexture = TextureManager::getSingleton().createManual(
      name,
      ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
      TEX_TYPE_2D,
      w, h,
      num_mipmaps,
      format,
      TU_RENDERTARGET,
      nullptr,
      hw_gamma_correction,
      multisample);
  ShimInOculusTexture(mMirrorTexture.get(), mOvrMirrorTexture);

  static const int num_threads = 1;
  static const auto culling_method = Ogre::INSTANCING_CULLING_SINGLETHREAD;
  mMirrorSceneManager = root->createSceneManager(
      "DefaultSceneManager", num_threads, culling_method);
  auto root_node = mMirrorSceneManager->getRootSceneNode();
  auto camera = mMirrorSceneManager->createCamera("DummyCamera");
  camera->setPosition(Vector3(0, 0, 0.5f));
  camera->setNearClipDistance(0.1f);
  camera->setFarClipDistance(10000.0f);
  camera->lookAt(Vector3(0, 0, 0));

  auto quad = mMirrorSceneManager->createManualObject();
  root_node->createChildSceneNode()->attachObject(quad);
  quad->setUseIdentityView(true);
  quad->setUseIdentityProjection(true);

  quad->begin("OculusSdkMirror", RenderOperation::OT_TRIANGLE_STRIP);

  // ll
  quad->position(-1.0f, -1.0f, 0.0f);
  quad->textureCoord(0.0f, 2.0f);
  // lr
  quad->position( 3.0f, -1.0f, 0.0f);
  quad->textureCoord(2.0f, 2.0f);
  // ul
  quad->position(-1.0f, 3.0f, 0.0f);
  quad->textureCoord(0.0f, 0.0f);

  quad->index(0);
  quad->index(1);
  quad->index(2);

  quad->end();

  *quad->_getObjectData().mLocalAabb = ArrayAabb::BOX_INFINITE;
  *quad->_getObjectData().mWorldAabb = ArrayAabb::BOX_INFINITE;

  auto compositor_manager = root->getCompositorManager2();
  if (!compositor_manager->hasWorkspaceDefinition("OculusSdkMirror")) {
    compositor_manager->createBasicWorkspaceDef(
        "OculusSdkMirror", ColourValue(0.0f, 0.0f, 0.0f, 1.0f));
  }
  mMirrorWorkspace = compositor_manager->addWorkspace(
      mMirrorSceneManager,
      render_window,
      camera,
      "OculusSdkMirror", true);
 ;
}

static Ogre::Matrix4& ToOgreMatrix(
    const ovrMatrix4f& src, Ogre::Matrix4& dst)
{
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      dst[i][j] = src.M[i][j];
    }
  }

  return dst;
}

void VR::Render(Ogre::SceneNode* head_displacement_node,
                Ogre::Camera** cameras,
                Ogre::Root* root)
{
  using namespace Ogre;

  // this is the primary 3D layer, later on we might want to lower the quality
  // of this for performance and introduce other high quality layers, such as
  // text.
  ovrLayerEyeFov primary_layer;
  primary_layer.Header.Type = ovrLayerType_EyeFov;
  primary_layer.Header.Flags =
      /* ovrLayerFlag_HighQuality | */
      /* ovrLayerFlag_TextureOriginAtBottomLeft | */
      0;

  auto frame_timing = ovr_GetFrameTiming(mHmd, 0);
  auto ts = ovr_GetTrackingState(mHmd, frame_timing.DisplayMidpointSeconds);
  const auto& head_pose = ts.HeadPose.ThePose;

  ovrEyeRenderDesc eye_render_descs[2];
  ovrVector3f hmd_to_eye_view_offsets[2];
  for (int i = 0; i < ovrEye_Count; ++i) {
    eye_render_descs[i] = ovr_GetRenderDesc(
        mHmd, (ovrEyeType)i, mEyes[i].Fov);
    hmd_to_eye_view_offsets[i] = eye_render_descs[i].HmdToEyeViewOffset;
  }
  ovrPosef eye_poses[2];
  ovr_CalcEyePoses(head_pose, hmd_to_eye_view_offsets, eye_poses);

  for (int i = 0; i < ovrEye_Count; ++i) {
    auto& eye = mEyes[(ovrEyeType)i];
    const auto& render_desc = eye_render_descs[i];
    const auto& hmd_to_eye_view_offset = hmd_to_eye_view_offsets[i];
    const auto& eye_pose = eye_poses[i];

    const auto& orient = eye_pose.Orientation;
    Quaternion orientation(orient.w, orient.x, orient.y, orient.z);
    const auto& pos = eye_pose.Position;
    Vector3 position(pos.x, pos.y, pos.z);
    
    head_displacement_node->setPosition(position);
    head_displacement_node->setOrientation(orientation);

    auto camera = cameras[i];
    
    unsigned int projection_modifiers =
        /* ovrProjection_None | */
        ovrProjection_RightHanded |
        /* ovrProjection_FarLessThanNear | */
        /* ovrProjection_FarClipAtInfinity | */
        ovrProjection_ClipRangeOpenGL |
        0;
    ovrMatrix4f projection = ovrMatrix4f_Projection(
        eye.Fov, 0.01f, 5000.0f, projection_modifiers);
    Matrix4 m;
    camera->setCustomProjectionMatrix(true, ToOgreMatrix(projection, m));

    Matrix4 T_world;
    T_world.makeTrans(-camera->getDerivedPosition());
    Matrix4 R(camera->getDerivedOrientation().Inverse());
    Matrix4 T_eye;
    const auto& eye_off = hmd_to_eye_view_offset;
    Vector3 eye_offset(eye_off.x, eye_off.y, eye_off.z);
    T_eye.makeTrans(-eye_offset);
    Matrix4 view = T_eye * R * T_world;
    camera->setCustomViewMatrix(true, view);

    int count = eye.SwapTextureSet->TextureCount;
    auto& current_index = eye.SwapTextureSet->CurrentIndex;
    current_index = (current_index + 1) % count;
    int other_index = (current_index + 1) % count;

    auto& slot = eye.Textures[current_index];
    auto& other_slot = eye.Textures[other_index];

    eye.CompositorWorkspaces[current_index]->setEnabled(true);
    eye.CompositorWorkspaces[other_index]->setEnabled(false);

    primary_layer.ColorTexture[i] = eye.SwapTextureSet;
    primary_layer.Viewport[i].Pos = {0, 0};
    primary_layer.Viewport[i].Size = {
      (int)slot.Texture->getWidth(), (int)slot.Texture->getHeight()
    };
    primary_layer.Fov[i] = eye.Fov;
    primary_layer.RenderPose[i] = eye_pose;
  }

  mMirrorWorkspace->setEnabled(false);
  root->renderOneFrame();

  ovrLayerHeader* layers[1] = {&primary_layer.Header};
  auto result = ovr_SubmitFrame(mHmd, 0, nullptr, layers, 1);

  mEyes[0].CompositorWorkspaces[0]->setEnabled(false);
  mEyes[0].CompositorWorkspaces[1]->setEnabled(false);
  mEyes[1].CompositorWorkspaces[0]->setEnabled(false);
  mEyes[1].CompositorWorkspaces[1]->setEnabled(false);
  mMirrorWorkspace->setEnabled(true);
  root->renderOneFrame();

}

void VR::RecenterHmd() {
  ovr_RecenterPose(mHmd);
}
