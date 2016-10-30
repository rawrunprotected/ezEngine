#include <PCH.h>
#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualShaderPin, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualShaderPin::ezVisualShaderPin(Type type, const ezVisualShaderPinDescriptor* pDescriptor, const ezDocumentObject* pObject)
  : ezPin(type, pDescriptor->m_sName, pObject)
{
  m_pDescriptor = pDescriptor;
}


const ezRTTI* ezVisualShaderPin::GetDataType() const
{
  return m_pDescriptor->m_pDataType;
}


const ezColorGammaUB& ezVisualShaderPin::GetColor() const
{
  return m_pDescriptor->m_Color;
}


const ezString& ezVisualShaderPin::GetTooltip() const
{
  return m_pDescriptor->m_sTooltip;
}

//////////////////////////////////////////////////////////////////////////
// ezVisualShaderNodeManager
//////////////////////////////////////////////////////////////////////////

bool ezVisualShaderNodeManager::InternalIsNode(const ezDocumentObject* pObject) const
{
  return pObject->GetType()->IsDerivedFrom(ezVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType());
}

void ezVisualShaderNodeManager::InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node)
{
  const auto* pDesc = ezVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pObject->GetType());

  if (pDesc == nullptr)
    return;

  node.m_Inputs.Reserve(pDesc->m_InputPins.GetCount());
  node.m_Outputs.Reserve(pDesc->m_OutputPins.GetCount());

  for (const auto& pin : pDesc->m_InputPins)
  {
    ezVisualShaderPin* pPin = EZ_DEFAULT_NEW(ezVisualShaderPin, ezPin::Type::Input, &pin, pObject);
    node.m_Inputs.PushBack(pPin);
  }

  for (const auto& pin : pDesc->m_OutputPins)
  {
    ezVisualShaderPin* pPin = EZ_DEFAULT_NEW(ezVisualShaderPin, ezPin::Type::Output, &pin, pObject);
    node.m_Outputs.PushBack(pPin);
  }
}

void ezVisualShaderNodeManager::InternalDestroyPins(const ezDocumentObject* pObject, NodeInternal& node)
{
  for (ezPin* pPin : node.m_Inputs)
  {
    EZ_DEFAULT_DELETE(pPin);
  }
  node.m_Inputs.Clear();

  for (ezPin* pPin : node.m_Outputs)
  {
    EZ_DEFAULT_DELETE(pPin);
  }
  node.m_Outputs.Clear();
}


void ezVisualShaderNodeManager::GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const
{
  const ezRTTI* pNodeBaseType = ezVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

  for (auto it = ezRTTI::GetFirstInstance(); it != nullptr; it = it->GetNextInstance())
  {
    if (it->IsDerivedFrom(pNodeBaseType) && !it->GetTypeFlags().IsSet(ezTypeFlags::Abstract))
      Types.PushBack(it);
  }
}

ezStatus ezVisualShaderNodeManager::InternalCanConnect(const ezPin* pSource, const ezPin* pTarget) const
{
  const ezVisualShaderPin* pPinSource = ezDynamicCast<const ezVisualShaderPin*>(pSource);
  const ezVisualShaderPin* pPinTarget = ezDynamicCast<const ezVisualShaderPin*>(pTarget);

  EZ_ASSERT_DEBUG(pPinSource != nullptr && pPinTarget != nullptr, "Das ist eigentlich unmoeglich!");

  //if (pPinSource->GetDataType() != pPinTarget->GetDataType())
  //{
  //  return ezStatus("Incompatible data types");
  //}

  if (!pTarget->GetConnections().IsEmpty())
    return ezStatus("Only one connection can be made to an input pin!");

  return ezStatus(EZ_SUCCESS);
}

const char* ezVisualShaderNodeManager::GetTypeCategory(const ezRTTI* pRtti) const
{
  const ezVisualShaderNodeDescriptor* pDesc = ezVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pRtti);

  if (pDesc == nullptr)
    return nullptr;

  return pDesc->m_sCategory;
}