#pragma once

#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class ezVisualShaderPinDescriptor;

class ezVisualShaderPin : public ezPin
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualShaderPin, ezPin);
public:

  ezVisualShaderPin(Type type, const ezVisualShaderPinDescriptor* pDescriptor, const ezDocumentObject* pObject);

  const ezRTTI* GetDataType() const;
  const ezColorGammaUB& GetColor() const;
  const ezString& GetTooltip() const;
  const ezVisualShaderPinDescriptor* GetDescriptor() const { return m_pDescriptor; }

private:
  const ezVisualShaderPinDescriptor* m_pDescriptor;
};

class ezVisualShaderNodeManager : public ezDocumentNodeManager
{
public:
  virtual bool InternalIsNode(const ezDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node) override;
  virtual void InternalDestroyPins(const ezDocumentObject* pObject, NodeInternal& node) override;
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const override;

  virtual ezStatus InternalCanConnect(const ezPin* pSource, const ezPin* pTarget) const override;
  virtual const char* GetTypeCategory(const ezRTTI* pRtti) const override;

};