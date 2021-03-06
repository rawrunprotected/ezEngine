#include <CorePCH.h>

#include <Core/World/World.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezWorldModule, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezWorldModule::ezWorldModule(ezWorld* pWorld)
    : m_pWorld(pWorld)
{
}

ezWorldModule::~ezWorldModule() {}

// protected methods

void ezWorldModule::RegisterUpdateFunction(const UpdateFunctionDesc& desc)
{
  m_pWorld->RegisterUpdateFunction(desc);
}

void ezWorldModule::DeregisterUpdateFunction(const UpdateFunctionDesc& desc)
{
  m_pWorld->DeregisterUpdateFunction(desc);
}

ezAllocatorBase* ezWorldModule::GetAllocator()
{
  return m_pWorld->GetAllocator();
}

ezInternal::WorldLargeBlockAllocator* ezWorldModule::GetBlockAllocator()
{
  return m_pWorld->GetBlockAllocator();
}

bool ezWorldModule::GetWorldSimulationEnabled() const
{
  return m_pWorld->GetWorldSimulationEnabled();
}

void ezWorldModule::InitializeInternal()
{
  Initialize();
}

void ezWorldModule::DeinitializeInternal()
{
  Deinitialize();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Core, WorldModuleFactory)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Reflection"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezPlugin::s_PluginEvents.AddEventHandler(ezWorldModuleFactory::PluginEventHandler);
    ezWorldModuleFactory::GetInstance()->FillBaseTypeIds();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezPlugin::s_PluginEvents.RemoveEventHandler(ezWorldModuleFactory::PluginEventHandler);
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

static ezUInt16 s_uiNextTypeId = 0;

ezWorldModuleFactory::ezWorldModuleFactory() {}

// static
ezWorldModuleFactory* ezWorldModuleFactory::GetInstance()
{
  static ezWorldModuleFactory* pInstance = new ezWorldModuleFactory();
  return pInstance;
}

ezUInt16 ezWorldModuleFactory::GetTypeId(const ezRTTI* pRtti)
{
  ezUInt16 uiTypeId = 0xFFFF;
  m_TypeToId.TryGetValue(pRtti, uiTypeId);
  return uiTypeId;
}

ezWorldModule* ezWorldModuleFactory::CreateWorldModule(ezUInt16 typeId, ezWorld* pWorld)
{
  if (typeId < m_CreatorFuncs.GetCount())
  {
    CreatorFunc func = m_CreatorFuncs[typeId];
    return (*func)(pWorld->GetAllocator(), pWorld);
  }

  return nullptr;
}

ezUInt16 ezWorldModuleFactory::RegisterWorldModule(const ezRTTI* pRtti, CreatorFunc creatorFunc)
{
  EZ_ASSERT_DEV(pRtti != ezGetStaticRTTI<ezWorldModule>(), "Trying to register a world module that is not reflected!");

  ezUInt16 uiTypeId = -1;
  if (m_TypeToId.TryGetValue(pRtti, uiTypeId))
  {
    return uiTypeId;
  }

  uiTypeId = s_uiNextTypeId++;
  m_TypeToId.Insert(pRtti, uiTypeId);

  m_CreatorFuncs.EnsureCount(uiTypeId + 1);

  m_CreatorFuncs[uiTypeId] = creatorFunc;

  return uiTypeId;
}

// static
void ezWorldModuleFactory::PluginEventHandler(const ezPlugin::PluginEvent& EventData)
{
  if (EventData.m_EventType == ezPlugin::PluginEvent::AfterLoadingBeforeInit)
  {
    ezWorldModuleFactory::GetInstance()->FillBaseTypeIds();
  }

  if (EventData.m_EventType == ezPlugin::PluginEvent::AfterUnloading)
  {
    ezWorldModuleFactory::GetInstance()->ClearUnloadedTypeToIDs();
  }
}

namespace
{
  struct NewEntry
  {
    EZ_DECLARE_POD_TYPE();

    const ezRTTI* m_pRtti;
    ezUInt16 m_uiTypeId;
  };

  static ezDynamicArray<NewEntry, ezStaticAllocatorWrapper> newEntries;
} // namespace

void ezWorldModuleFactory::FillBaseTypeIds()
{
  const ezRTTI* pModuleRtti = ezGetStaticRTTI<ezWorldModule>();

  for (auto it = m_TypeToId.GetIterator(); it.IsValid(); ++it)
  {
    const ezRTTI* pRtti = it.Key();
    if (!pRtti->IsDerivedFrom<ezComponent>()) // is not a component manager entry
    {
      ezUInt16 uiTypeId = it.Value();
      const ezRTTI* pParentRtti = pRtti->GetParentType();
      while (pParentRtti != pModuleRtti)
      {
        if (!m_TypeToId.Contains(pParentRtti))
        {
          auto& newEntry = newEntries.ExpandAndGetRef();
          newEntry.m_pRtti = pParentRtti;
          newEntry.m_uiTypeId = uiTypeId;
        }

        pParentRtti = pParentRtti->GetParentType();
      }
    }
  }

  for (auto& newEntry : newEntries)
  {
    m_TypeToId.Insert(newEntry.m_pRtti, newEntry.m_uiTypeId);
  }
  newEntries.Clear();
}

void ezWorldModuleFactory::ClearUnloadedTypeToIDs()
{
  ezSet<const ezRTTI*> allRttis;

  for (const ezRTTI* pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    allRttis.Insert(pRtti);
  }

  for (auto it = m_TypeToId.GetIterator(); it.IsValid();)
  {
    const ezRTTI* pRtti = it.Key();

    if (!allRttis.Contains(pRtti))
    {
      it = m_TypeToId.Remove(it);
    }
    else
    {
      ++it;
    }
  }
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_WorldModule);

