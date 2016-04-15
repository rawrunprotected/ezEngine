#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Types/Variant.h>

class ezVisualizerAttribute;
class ezDocumentObject;
struct ezDocumentObjectPropertyEvent;
struct ezQtDocumentWindowEvent;

class EZ_EDITORFRAMEWORK_DLL ezVisualizerAdapter
{
public:
  ezVisualizerAdapter();
  virtual ~ezVisualizerAdapter();

  void SetVisualizer(const ezVisualizerAttribute* pAttribute, const ezDocumentObject* pObject);

private:
  void DocumentObjectPropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void DocumentWindowEventHandler(const ezQtDocumentWindowEvent& e);

protected:
  virtual ezTransform GetObjectTransform() const;

  virtual void Finalize() = 0;
  virtual void Update() = 0;
  virtual void UpdateGizmoTransform() = 0;

  const ezVisualizerAttribute* m_pVisualizerAttr;
  const ezDocumentObject* m_pObject;
};