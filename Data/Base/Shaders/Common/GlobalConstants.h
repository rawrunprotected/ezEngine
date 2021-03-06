#pragma once

#include "Platforms.h"
#include "ConstantBufferMacros.h"

#define WIREFRAME_RENDER_PASS_COLOR 1
#define WIREFRAME_RENDER_PASS_MONOCHROME 2

#define EDITOR_RENDER_PASS_LIT_ONLY 1
#define EDITOR_RENDER_PASS_LIGHT_COUNT 2
#define EDITOR_RENDER_PASS_DECAL_COUNT 3
#define	EDITOR_RENDER_PASS_TEXCOORDS_UV0 4
#define	EDITOR_RENDER_PASS_VERTEX_NORMALS 5
#define	EDITOR_RENDER_PASS_VERTEX_TANGENTS 6
#define	EDITOR_RENDER_PASS_PIXEL_NORMALS 7
#define	EDITOR_RENDER_PASS_DIFFUSE_COLOR 8
#define	EDITOR_RENDER_PASS_DIFFUSE_COLOR_RANGE 9
#define	EDITOR_RENDER_PASS_SPECULAR_COLOR 10
#define	EDITOR_RENDER_PASS_EMISSIVE_COLOR 11
#define	EDITOR_RENDER_PASS_ROUGHNESS 12
#define	EDITOR_RENDER_PASS_OCCLUSION 13
#define	EDITOR_RENDER_PASS_DEPTH 14
#define EDITOR_RENDER_PASS_STATIC_VS_DYNAMIC 15
#define EDITOR_RENDER_PASS_BONE_WEIGHTS 16

CONSTANT_BUFFER(ezGlobalConstants, 0)
{
  // Use functions from CameraConstantsAccess.h to access these and derived camera properties.
  MAT4(CameraToScreenMatrix)[2];
  MAT4(ScreenToCameraMatrix)[2];
  MAT4(WorldToCameraMatrix)[2];
  MAT4(CameraToWorldMatrix)[2];
  MAT4(WorldToScreenMatrix)[2];
  MAT4(ScreenToWorldMatrix)[2];

  FLOAT4(ViewportSize);   // x = width, y = height, z = 1 / width, w = 1 / height
  FLOAT4(ClipPlanes);     // x = near, y = far, z = 1 / far

  FLOAT1(DeltaTime);
  FLOAT1(GlobalTime);
  FLOAT1(WorldTime);

  FLOAT1(Exposure);

  INT1(RenderPass);
  UINT1(NumMsaaSamples);
};

#include "CameraConstantsAccess.h"
