#include <RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Messages/SetColorMessage.h>
#include <RendererFoundation/Device/Device.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgSetMeshMaterial);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgSetMeshMaterial, 1, ezRTTIDefaultAllocator<ezMsgSetMeshMaterial>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new ezAssetBrowserAttribute("Material")),
    EZ_MEMBER_PROPERTY("MaterialSlot", m_uiMaterialSlot),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezAutoGenVisScriptMsgSender,
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezMsgSetMeshMaterial::SetMaterialFile(const char* szFile)
{
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    m_hMaterial = ezResourceManager::LoadResource<ezMaterialResource>(szFile);
  }
  else
  {
    m_hMaterial.Invalidate();
  }
}

const char* ezMsgSetMeshMaterial::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

void ezMsgSetMeshMaterial::Serialize(ezStreamWriter& stream) const
{
  // has to be stringyfied for transfer
  stream << GetMaterialFile();
  stream << m_uiMaterialSlot;
}

void ezMsgSetMeshMaterial::Deserialize(ezStreamReader& stream, ezUInt8 uiTypeVersion)
{
  ezStringBuilder file;
  stream >> file;
  SetMaterialFile(file);

  stream >> m_uiMaterialSlot;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshRenderData, 1, ezRTTIDefaultAllocator<ezMeshRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezMeshComponentBase, 1)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgSetMeshMaterial, OnSetMaterial),
    EZ_MESSAGE_HANDLER(ezMsgSetColor, OnSetColor),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

ezMeshComponentBase::ezMeshComponentBase()
{
  m_RenderDataCategory = ezInvalidRenderDataCategory;
  m_Color = ezColor::White;
}

ezMeshComponentBase::~ezMeshComponentBase() {}

void ezMeshComponentBase::OnDeactivated()
{
  if (!m_hSkinningTransformsBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hSkinningTransformsBuffer);
    m_hSkinningTransformsBuffer.Invalidate();
  }
}

void ezMeshComponentBase::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  // ignore components that have created meshes (?)

  s << m_hMesh;

  ezUInt32 uiCategory = m_RenderDataCategory.m_uiValue;
  s << uiCategory;

  s << m_Materials.GetCount();

  for (const auto& mat : m_Materials)
  {
    s << mat;
  }

  s << m_Color;
}

void ezMeshComponentBase::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = stream.GetStream();

  s >> m_hMesh;

  ezUInt32 uiCategory = 0;
  s >> uiCategory;
  m_RenderDataCategory.m_uiValue = uiCategory;

  ezUInt32 uiMaterials = 0;
  s >> uiMaterials;

  m_Materials.SetCount(uiMaterials);

  for (auto& mat : m_Materials)
  {
    s >> mat;
  }

  s >> m_Color;
}

ezResult ezMeshComponentBase::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  if (m_hMesh.IsValid())
  {
    ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowFallback);
    bounds = pMesh->GetBounds();
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezMeshComponentBase::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  const ezUInt32 uiMeshIDHash = m_hMesh.GetResourceIDHash();

  const ezUInt32 uiFlipWinding = GetOwner()->GetGlobalTransformSimd().ContainsNegativeScale() ? 1 : 0;
  const ezUInt32 uiUniformScale = GetOwner()->GetGlobalTransformSimd().ContainsUniformScale() ? 1 : 0;

  ezResourceLock<ezMeshResource> pMesh(m_hMesh, ezResourceAcquireMode::AllowFallback);
  ezArrayPtr<const ezMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (ezUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const ezUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    ezMaterialResourceHandle hMaterial;

    // If we have a material override, use that otherwise use the default mesh material.
    if (GetMaterial(uiMaterialIndex).IsValid())
      hMaterial = m_Materials[uiMaterialIndex];
    else
      hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    const ezUInt32 uiMaterialIDHash = hMaterial.IsValid() ? hMaterial.GetResourceIDHash() : 0;

    // Generate batch id from mesh, material and part index.
    ezUInt32 data[] = {uiMeshIDHash, uiMaterialIDHash, uiPartIndex, uiFlipWinding};

    if (!m_SkinningMatrices.IsEmpty())
    {
      // TODO: When skinning is enabled, batching is prevented. Review this.
      data[2] = this->GetUniqueIdForRendering();
    }

    ezUInt32 uiBatchId = ezHashingUtils::xxHash32(data, sizeof(data));

    ezMeshRenderData* pRenderData = CreateRenderData(uiBatchId);
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh = m_hMesh;
      pRenderData->m_hMaterial = hMaterial;
      pRenderData->m_Color = m_Color;

      pRenderData->m_uiSubMeshIndex = uiPartIndex;
      pRenderData->m_uiFlipWinding = uiFlipWinding;
      pRenderData->m_uiUniformScale = uiUniformScale;

      pRenderData->m_uiUniqueID = GetUniqueIdForRendering(uiMaterialIndex);

      if (!m_SkinningMatrices.IsEmpty())
      {
        pRenderData->m_hSkinningMatrices = m_hSkinningTransformsBuffer;
        pRenderData->m_pNewSkinningMatricesData = ezArrayPtr<const ezUInt8>(reinterpret_cast<const ezUInt8*>(m_SkinningMatrices.GetPtr()),
                                                                            m_SkinningMatrices.GetCount() * sizeof(ezMat4));
      }
    }

    // Determine render data category.
    ezRenderData::Category category = m_RenderDataCategory;
    if (category == ezInvalidRenderDataCategory)
    {
      if (hMaterial.IsValid())
      {
        ezResourceLock<ezMaterialResource> pMaterial(hMaterial, ezResourceAcquireMode::AllowFallback);
        ezTempHashedString blendModeValue = pMaterial->GetPermutationValue("BLEND_MODE");
        if (blendModeValue == "BLEND_MODE_OPAQUE" || blendModeValue == "")
        {
          category = ezDefaultRenderDataCategories::LitOpaque;
        }
        else if (blendModeValue == "BLEND_MODE_MASKED")
        {
          category = ezDefaultRenderDataCategories::LitMasked;
        }
        else
        {
          category = ezDefaultRenderDataCategories::LitTransparent;
        }
      }
      else
      {
        category = ezDefaultRenderDataCategories::LitOpaque;
      }
    }

    // Sort by material and then by mesh
    ezUInt32 uiSortingKey = (uiMaterialIDHash << 16) | (uiMeshIDHash & 0xFFFE) | uiFlipWinding;
    msg.AddRenderData(pRenderData, category, uiSortingKey, ezRenderData::Caching::IfStatic);
  }
}

void ezMeshComponentBase::SetMesh(const ezMeshResourceHandle& hMesh)
{
  m_hMesh = hMesh;

  TriggerLocalBoundsUpdate();
}

void ezMeshComponentBase::SetMaterial(ezUInt32 uiIndex, const ezMaterialResourceHandle& hMaterial)
{
  m_Materials.EnsureCount(uiIndex + 1);

  m_Materials[uiIndex] = hMaterial;
}

ezMaterialResourceHandle ezMeshComponentBase::GetMaterial(ezUInt32 uiIndex) const
{
  if (uiIndex >= m_Materials.GetCount())
    return ezMaterialResourceHandle();

  return m_Materials[uiIndex];
}

void ezMeshComponentBase::SetMeshFile(const char* szFile)
{
  ezMeshResourceHandle hMesh;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hMesh = ezResourceManager::LoadResource<ezMeshResource>(szFile);
  }

  SetMesh(hMesh);
}

const char* ezMeshComponentBase::GetMeshFile() const
{
  if (!m_hMesh.IsValid())
    return "";

  return m_hMesh.GetResourceID();
}

void ezMeshComponentBase::SetColor(const ezColor& color)
{
  m_Color = color;
}

const ezColor& ezMeshComponentBase::GetColor() const
{
  return m_Color;
}

void ezMeshComponentBase::OnSetMaterial(ezMsgSetMeshMaterial& msg)
{
  SetMaterial(msg.m_uiMaterialSlot, msg.m_hMaterial);
}

void ezMeshComponentBase::OnSetColor(ezMsgSetColor& msg)
{
  msg.ModifyColor(m_Color);
}

ezMeshRenderData* ezMeshComponentBase::CreateRenderData(ezUInt32 uiBatchId) const
{
  return ezCreateRenderDataForThisFrame<ezMeshRenderData>(GetOwner(), uiBatchId);
}

ezUInt32 ezMeshComponentBase::Materials_GetCount() const
{
  return m_Materials.GetCount();
}


const char* ezMeshComponentBase::Materials_GetValue(ezUInt32 uiIndex) const
{
  auto hMat = GetMaterial(uiIndex);

  if (!hMat.IsValid())
    return "";

  return hMat.GetResourceID();
}


void ezMeshComponentBase::Materials_SetValue(ezUInt32 uiIndex, const char* value)
{
  if (ezStringUtils::IsNullOrEmpty(value))
    SetMaterial(uiIndex, ezMaterialResourceHandle());
  else
  {
    auto hMat = ezResourceManager::LoadResource<ezMaterialResource>(value);
    SetMaterial(uiIndex, hMat);
  }
}


void ezMeshComponentBase::Materials_Insert(ezUInt32 uiIndex, const char* value)
{
  ezMaterialResourceHandle hMat;

  if (!ezStringUtils::IsNullOrEmpty(value))
    hMat = ezResourceManager::LoadResource<ezMaterialResource>(value);

  m_Materials.Insert(hMat, uiIndex);
}


void ezMeshComponentBase::Materials_Remove(ezUInt32 uiIndex)
{
  m_Materials.RemoveAtAndCopy(uiIndex);
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshComponentBase);

