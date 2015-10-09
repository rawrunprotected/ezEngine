#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>


class ezScenegraphPanel : public ezDocumentPanel
{
  Q_OBJECT

public:
  ezScenegraphPanel(QWidget* pParent, ezSceneDocument* pDocument);
  ~ezScenegraphPanel();

  static void RegisterActions();

private slots:
  void OnItemDoubleClicked(const QModelIndex&);
  void OnRequestContextMenu(QPoint pos);

private:
  void DocumentSceneEventHandler(const ezSceneDocument::SceneEvent& e);

  ezSceneDocument* m_pDocument;
  ezRawDocumentTreeWidget* m_pTreeWidget;
};