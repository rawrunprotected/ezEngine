#include <PCH.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <QStringList>
#include <QMimeData>
#include <QMessageBox>

ezQtDocumentTreeModel::ezQtDocumentTreeModel(const ezDocumentObjectManager* pTree, const ezRTTI* pBaseClass, const char* szChildProperty) :
  QAbstractItemModel(nullptr)
{
  m_pDocumentTree = pTree;
  m_pBaseClass = pBaseClass;
  m_sChildProperty = szChildProperty;
  m_PropertyPath = szChildProperty;
  auto pProp = m_pBaseClass->FindPropertyByName(m_sChildProperty);
  EZ_ASSERT_DEV(pProp != nullptr && (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set),
    "The visualized object property tree must either be a set or array!");
  EZ_ASSERT_DEV(!pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) || pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner),
    "The visualized object must have ownership of the property objects!");

  m_pDocumentTree->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezQtDocumentTreeModel::TreeEventHandler, this));
  m_pDocumentTree->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtDocumentTreeModel::TreePropertyEventHandler, this));
}

ezQtDocumentTreeModel::~ezQtDocumentTreeModel()
{
  m_pDocumentTree->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezQtDocumentTreeModel::TreeEventHandler, this));
  m_pDocumentTree->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtDocumentTreeModel::TreePropertyEventHandler, this));
}

void ezQtDocumentTreeModel::TreePropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_sPropertyPath != "Name")
    return;

  auto index = ComputeModelIndex(e.m_pObject);
  QVector<int> v;
  v.push_back(Qt::DisplayRole);
  dataChanged(index, index, v);
}

void ezQtDocumentTreeModel::TreeEventHandler(const ezDocumentObjectStructureEvent& e)
{
  if (!e.m_pObject->GetTypeAccessor().GetType()->IsDerivedFrom(m_pBaseClass))
    return;
  if (!e.m_sParentProperty.IsEqual(m_sChildProperty) && !e.m_sParentProperty.IsEqual("Children"))
    return;

  // TODO: BLA root object could have other objects instead of m_pBaseClass, in which case indices are broken on root.

  switch (e.m_EventType)
  {
  case ezDocumentObjectStructureEvent::Type::BeforeObjectAdded:
    {
      ezInt32 iIndex = (ezInt32)e.m_PropertyIndex.ConvertTo<ezInt32>();
      if (e.m_pNewParent == m_pDocumentTree->GetRootObject())
        beginInsertRows(QModelIndex(), iIndex, iIndex);
      else
        beginInsertRows(ComputeModelIndex(e.m_pNewParent), iIndex, iIndex);
    }
    break;
  case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
    {
      endInsertRows();
    }
    break;
  case ezDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    {
      ezInt32 iIndex = ComputeIndex(e.m_pObject);

      beginRemoveRows(ComputeParent(e.m_pObject), iIndex, iIndex);
    }
    break;
  case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      endRemoveRows();
    }
    break;
  case ezDocumentObjectStructureEvent::Type::BeforeObjectMoved:
    {
      ezInt32 iNewIndex = (ezInt32)e.m_PropertyIndex.ConvertTo<ezInt32>();
      ezInt32 iIndex = ComputeIndex(e.m_pObject);
      beginMoveRows(ComputeModelIndex(e.m_pPreviousParent), iIndex, iIndex, ComputeModelIndex(e.m_pNewParent), iNewIndex);
    }
    break;
  case ezDocumentObjectStructureEvent::Type::AfterObjectMoved:
    {
      endMoveRows();
    }
    break;
  }
}

QModelIndex ezQtDocumentTreeModel::index(int row, int column, const QModelIndex& parent) const
{
  if (!parent.isValid())
  {
    if (m_pDocumentTree->GetRootObject()->GetChildren().IsEmpty())
      return QModelIndex();

    ezVariant value = m_pDocumentTree->GetRootObject()->GetTypeAccessor().GetValue("Children", row);
    EZ_ASSERT_DEV(value.IsValid() && value.IsA<ezUuid>(), "Tree corruption!");

    const ezDocumentObject* pObject = m_pDocumentTree->GetObject(value.Get<ezUuid>());
    return createIndex(row, column, const_cast<ezDocumentObject*>(pObject));
  }

  const ezDocumentObject* pParent = (const ezDocumentObject*) parent.internalPointer();
  ezVariant value = pParent->GetTypeAccessor().GetValue(m_PropertyPath, row);
  EZ_ASSERT_DEV(value.IsValid() && value.IsA<ezUuid>(), "Tree corruption!");
  const ezDocumentObject* pChild = m_pDocumentTree->GetObject(value.Get<ezUuid>());

  return createIndex(row, column, const_cast<ezDocumentObject*>(pChild));
}

ezInt32 ezQtDocumentTreeModel::ComputeIndex(const ezDocumentObject* pObject) const
{
  const ezDocumentObject* pParent = pObject->GetParent();
 
  ezInt32 iIndex = pObject->GetPropertyIndex().ConvertTo<ezInt32>();
  return iIndex;
}

QModelIndex ezQtDocumentTreeModel::ComputeModelIndex(const ezDocumentObject* pObject) const
{
  if (pObject == m_pDocumentTree->GetRootObject())
    return QModelIndex();

  return index(ComputeIndex(pObject), 0, ComputeParent(pObject));
}

QModelIndex ezQtDocumentTreeModel::ComputeParent(const ezDocumentObject* pObject) const
{
  const ezDocumentObject* pParent = pObject->GetParent();

  if (pParent == m_pDocumentTree->GetRootObject())
    return QModelIndex();

  ezInt32 iIndex = ComputeIndex(pParent);

  return createIndex(iIndex, 0, const_cast<ezDocumentObject*>(pParent));
}

QModelIndex ezQtDocumentTreeModel::parent(const QModelIndex& child) const
{
  const ezDocumentObject* pObject = (const ezDocumentObject*) child.internalPointer();

  return ComputeParent(pObject);
}

int ezQtDocumentTreeModel::rowCount(const QModelIndex& parent) const
{
  int iCount = 0;

  if (!parent.isValid())
  {
    iCount = m_pDocumentTree->GetRootObject()->GetTypeAccessor().GetCount("Children");
  }
  else
  {
    const ezDocumentObject* pObject = (const ezDocumentObject*) parent.internalPointer();
  
    iCount = pObject->GetTypeAccessor().GetCount(m_PropertyPath);
  }

  return iCount;
}

int ezQtDocumentTreeModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}

QVariant ezQtDocumentTreeModel::data(const QModelIndex& index, int role) const
{
  //if (index.isValid())
  {
    const ezDocumentObject* pObject = (const ezDocumentObject*) index.internalPointer();

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
      {
        return QString::fromUtf8(pObject->GetTypeAccessor().GetValue(ezToolsReflectionUtils::CreatePropertyPath("Name")).ConvertTo<ezString>().GetData());
      }
      break;
    }
  }

  return QVariant();
}

Qt::DropActions ezQtDocumentTreeModel::supportedDropActions() const
{
  return Qt::MoveAction | Qt::CopyAction;
}

Qt::ItemFlags ezQtDocumentTreeModel::flags(const QModelIndex &index) const
{
  if (!index.isValid())
    return Qt::ItemIsDropEnabled;

  if (index.column() == 0)
  {
    return (/*Qt::ItemIsUserCheckable | */ Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
  }

  return Qt::ItemFlag::NoItemFlags;
}

bool ezQtDocumentTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
  if (column > 0)
    return false;

  if (data->hasFormat("application/ezEditor.ObjectSelection"))
  {
    ezDocumentObject* pNewParent = (ezDocumentObject*) parent.internalPointer();

    ezHybridArray<ezDocumentObject*, 32> Dragged;

    QByteArray encodedData = data->data("application/ezEditor.ObjectSelection");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);

    int iIndices = 0;
    stream >> iIndices;

    for (int i = 0; i < iIndices; ++i)
    {
      void* p = nullptr;

      uint len = sizeof(void*);
      stream.readRawData((char*) &p, len);

      ezDocumentObject* pDocObject = (ezDocumentObject*) p;

      Dragged.PushBack(pDocObject);

      if (action != Qt::DropAction::MoveAction)
      {
        bool bCanMove = true;
        const ezDocumentObject* pCurParent = pNewParent;

        while (pCurParent)
        {
          if (pCurParent == pDocObject)
          {
            bCanMove = false;
            break;
          }

          pCurParent = pCurParent->GetParent();
        }

        if (!bCanMove)
        {
          ezUIServices::MessageBoxInformation("Cannot move an object to one of its own children");
          return false;
        }
      }
    }

    auto pDoc = m_pDocumentTree->GetDocument();
    auto pHistory = pDoc->GetCommandHistory();
    pHistory->StartTransaction();

    for (ezUInt32 i = 0; i < Dragged.GetCount(); ++i)
    {
      ezMoveObjectCommand cmd;
      cmd.m_Object = Dragged[i]->GetGuid();
      cmd.m_sParentProperty = m_sChildProperty;
      cmd.m_Index = row;

      if (pNewParent)
        cmd.m_NewParent = pNewParent->GetGuid();
      else
        cmd.m_sParentProperty = "Children";

      pHistory->AddCommand(cmd);
    }

    pHistory->FinishTransaction();

    return true;
  }

  if (data->hasFormat("application/ezEditor.ObjectCreator"))
  {
    QByteArray encodedData = data->data("application/ezEditor.ObjectCreator");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    QString typeName;
    stream >> typeName;

    ezAddObjectCommand cmd;
    cmd.SetType(typeName.toUtf8().data());
    cmd.m_sParentProperty = m_sChildProperty;
    cmd.m_Index = row;

    if (parent.isValid())
      cmd.m_Parent = ((ezDocumentObject*) parent.internalPointer())->GetGuid();

    auto history = m_pDocumentTree->GetDocument()->GetCommandHistory();

    history->StartTransaction();

    if (history->AddCommand(cmd).m_Result.Failed())
      history->CancelTransaction();
    else
      history->FinishTransaction();
  }

  return false;
}

QStringList ezQtDocumentTreeModel::mimeTypes() const
{
  QStringList types;
  types << "application/ezEditor.ObjectSelection";
  types << "application/ezEditor.ObjectCreator";
  return types;
}

QMimeData* ezQtDocumentTreeModel::mimeData(const QModelIndexList& indexes) const
{
  QMimeData* mimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  int iCount = 0;

  foreach(QModelIndex index, indexes)
  {
    if (index.isValid())
      ++iCount;
  }

  stream << iCount;

  foreach(QModelIndex index, indexes)
  {
    if (index.isValid())
    {
      void* pObject = index.internalPointer();
      stream.writeRawData((const char*) &pObject, sizeof(void*));
    }
  }

  mimeData->setData("application/ezEditor.ObjectSelection", encodedData);
  return mimeData;
}

bool ezQtDocumentTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if (role == Qt::EditRole)
  {
    const ezDocumentObject* pObject = (const ezDocumentObject*) index.internalPointer();

    auto pHistory = m_pDocumentTree->GetDocument()->GetCommandHistory();

    pHistory->StartTransaction();

    ezSetObjectPropertyCommand cmd;
    cmd.m_NewValue = value.toString().toUtf8().data();
    cmd.m_Object = pObject->GetGuid();
    cmd.SetPropertyPath("Name");

    pHistory->AddCommand(cmd);

    pHistory->FinishTransaction();

    QVector<int> roles;
    roles.push_back(Qt::DisplayRole);
    roles.push_back(Qt::EditRole);

    emit dataChanged(index, index, roles);
  }

  return false;
}

