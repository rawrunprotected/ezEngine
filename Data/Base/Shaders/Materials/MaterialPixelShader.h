#pragma once

#define USE_WORLDPOS

#include <Shaders/Common/Lighting.h>
#include <Shaders/Materials/MaterialHelper.h>

#if !defined(RENDER_PASS) || !defined(BLEND_MODE)
	#error "RENDER_PASS and BLEND_MODE permutations must be defined"
#endif

#if RENDER_PASS == RENDER_PASS_DEPTH_ONLY
	void main(PS_IN Input)
#else
	float4 main(PS_IN Input) : SV_Target
#endif
{
	float opacity = 1.0f;

	#if BLEND_MODE != BLEND_MODE_OPAQUE
		opacity = GetOpacity(Input);

		#if BLEND_MODE == BLEND_MODE_MASKED && RENDER_PASS != RENDER_PASS_WIREFRAME
			clip(opacity);
		#endif
	#endif

	ezMaterialData matData = FillMaterialData(Input);

	#if RENDER_PASS == RENDER_PASS_EDITOR
		if (RenderPass == EDITOR_RENDER_PASS_LIT_ONLY)
		{
			matData.diffuseColor = 0.5;
			matData.specularColor = 0.0;
		}
	#endif

	float3 litColor = CalculateLighting(matData);
  litColor += matData.emissiveColor;

	#if RENDER_PASS == RENDER_PASS_FORWARD
		#if defined(SHADING_MODE) && SHADING_MODE == SHADING_MODE_LIT
			return float4(litColor, opacity);
		#else
			return float4(matData.diffuseColor, opacity);
		#endif

	#elif RENDER_PASS == RENDER_PASS_EDITOR
		if (RenderPass == EDITOR_RENDER_PASS_LIT_ONLY)
		{
			return float4(litColor, 1);
		}
		else if (RenderPass == EDITOR_RENDER_PASS_TEXCOORDS_UV0)
		{
			#if defined(USE_TEXCOORD0)
				return float4(SrgbToLinear(float3(frac(Input.TexCoords.xy), 0)), 1);
			#else
				return float4(0, 0, 0, 1);
			#endif
		}
		else if (RenderPass == EDITOR_RENDER_PASS_NORMALS)
		{
			return float4(SrgbToLinear(matData.worldNormal * 0.5 + 0.5), 1);
		}
		else if (RenderPass == EDITOR_RENDER_PASS_DIFFUSE_COLOR)
		{
			return float4(matData.diffuseColor, 1);
		}
		else if (RenderPass == EDITOR_RENDER_PASS_DIFFUSE_COLOR_RANGE)
		{
			float luminance = GetLuminance(matData.diffuseColor);
			if (luminance < 0.017) // 40 srgb
			{
				return float4(1, 0, 1, 1);
			}
			else if (luminance > 0.9) // 243 srgb
			{
				return float4(0, 1, 0, 1);
			}

			return float4(matData.diffuseColor, 1);
		}
		else if (RenderPass == EDITOR_RENDER_PASS_SPECULAR_COLOR)
		{
			return float4(matData.specularColor, 1);
		}
    else if (RenderPass == EDITOR_RENDER_PASS_EMISSIVE_COLOR)
		{
			return float4(matData.emissiveColor, 1);
		}
		else if (RenderPass == EDITOR_RENDER_PASS_ROUGHNESS)
		{
			return float4(SrgbToLinear(matData.roughness), 1);
		}
    else if (RenderPass == EDITOR_RENDER_PASS_OCCLUSION)
		{
			return float4(SrgbToLinear(matData.occlusion), 1);
		}
		else if (RenderPass == EDITOR_RENDER_PASS_DEPTH)
		{
			// todo proper linearization
			float depth = 1.0 - saturate(Input.Position.z / Input.Position.w);
			depth = depth * depth * depth * depth;
			return float4(depth, depth, depth, 1);
		}
		else
		{
			return float4(1.0f, 0.0f, 1.0f, 1.0f);
		}

	#elif RENDER_PASS == RENDER_PASS_WIREFRAME
		if (RenderPass == WIREFRAME_RENDER_PASS_MONOCHROME)
		{
			return float4(0.4f, 0.4f, 0.4f, 1.0f);
		}
		else
		{
			return float4(matData.diffuseColor, 1.0f);
		}

	#elif RENDER_PASS == RENDER_PASS_PICKING
		return RGBA8ToFloat4(GetInstanceData(Input).GameObjectID);

	#endif
}