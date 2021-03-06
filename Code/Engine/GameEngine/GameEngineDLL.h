#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
  #ifdef BUILDSYSTEM_BUILDING_GAMEENGINE_LIB
    #define EZ_GAMEENGINE_DLL __declspec(dllexport)
  #else
    #define EZ_GAMEENGINE_DLL __declspec(dllimport)
  #endif
#else
  #define EZ_GAMEENGINE_DLL
#endif

