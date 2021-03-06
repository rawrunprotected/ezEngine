#cmake

#
# Copy this file into the folder of an engine library to add it to the solution.
# Below there are configuration options to add preprocessor definitions and link dependencies to the library.
#
# *** Modify this file as required ***
#

# Add all the preprocessor definitions that are needed to configure this library
# add_definitions (-DMY_PREPROCESS_DEFINITION_1)
# add_definitions (-DMY_PREPROCESS_DEFINITION_2)

# Add all the libraries that this project needs to link against here
# SET (LINK_LIBRARIES
#  Foundation      # Example: Another library build with cmake
#  OpenGL32.lib    # Example: A library that can be found in any lib path, but was not built with cmake
# )

SET (LINK_LIBRARIES
  Foundation
  Core
  System
  ThirdParty
  RendererFoundation
  RendererCore
  RendererDX11
  Texture
)

if (BUILDSYSTEM_PLATFORM_WINDOWS_UWP)
  LIST(APPEND LINK_LIBRARIES FileservePlugin)
endif()

if (BUILDSYSTEM_PLATFORM_WINDOWS_UWP AND EZ_ENABLE_MIXEDREALITY_SUPPORT)
  LIST(APPEND LINK_LIBRARIES WindowsMixedReality)
endif()

include ("${EZ_CMAKE_UTILS_PATH}/CMAKE_Library.txt")
