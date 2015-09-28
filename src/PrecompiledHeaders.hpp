#pragma once

////////////////////////////////////////////////////////////////////////////////
// standard C
////////////////////////////////////////////////////////////////////////////////
#include <cstdlib>
#include <cstdio>

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <tchar.h>
#endif

////////////////////////////////////////////////////////////////////////////////
// standard C++
////////////////////////////////////////////////////////////////////////////////
#include <string>
#include <vector>
#include <memory>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <chrono>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/clamp.hpp>

////////////////////////////////////////////////////////////////////////////////
// third party
////////////////////////////////////////////////////////////////////////////////

#include <Ogre.h>
#include <Plugins/ParticleFX/OgreParticleFXPlugin.h>
#include <RenderSystems/GL3Plus/OgreGL3PlusPlugin.h>

#include <OgreDepthBuffer.h>
#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/OgreCompositorWorkspace.h>

#include "OgreGL3PlusPrerequisites.h"
#include "OgreGL3PlusSupport.h"
#include "OgrePlatform.h"
#include "OgreRenderTexture.h"
#include "OgreTexture.h"
#include "OgreHardwarePixelBuffer.h"
// HACK
#define private public
#include <OgreGL3PlusTexture.h>
#undef private

////////////////////////////////////////////////////////////////////////////////

#include <SDL.h>
#include <SDL_syswm.h>

////////////////////////////////////////////////////////////////////////////////

#include <OVR_Version.h>
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>
#include <Extras/OVR_CAPI_Util.h>
