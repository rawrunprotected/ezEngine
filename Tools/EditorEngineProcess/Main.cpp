#include <PCH.h>
#include <EditorEngineProcess/Application.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Time/Clock.h>

EZ_APPLICATION_ENTRY_POINT(ezEditorProcessApp);

ezEditorProcessApp::ezEditorProcessApp()
{
  EnableMemoryLeakReporting(true);
  m_pApp = nullptr;
}

void ezEditorProcessApp::AfterEngineInit()
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  // Setting this flags prevents Windows from showing a dialog when the Engine process crashes
  // this also speeds up process termination significantly (down to less than a second)
  DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
  SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
#endif

  ezTelemetry::CreateServer();

  {
    ezStringBuilder sAppDir = ezOSFile::GetApplicationDirectory();
    sAppDir.AppendPath("../../../Shared/Tools/EditorEngineProcess");

    ezOSFile osf;
    osf.CreateDirectoryStructure(sAppDir);

    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
    ezFileSystem::AddDataDirectory("", ezFileSystem::AllowWrites, "App"); // for absolute paths
    ezFileSystem::AddDataDirectory(sAppDir.GetData(), ezFileSystem::AllowWrites, "App"); // for everything relative
  }

  ezPlugin::LoadPlugin("ezInspectorPlugin");
  ezPlugin::LoadPlugin("ezEnginePluginTest");
  EZ_VERIFY(ezPlugin::LoadPlugin("ezShaderCompilerHLSL").Succeeded(), "Compiler Plugin not found");

  int argc = GetArgumentCount();
  const char** argv = GetArgumentsArray();
  m_pApp = new QApplication(argc, (char**) argv);

  EZ_VERIFY(m_IPC.ConnectToHostProcess().Succeeded(), "Could not connect to host");

  m_IPC.m_Events.AddEventHandler(ezMakeDelegate(&ezEditorProcessApp::EventHandlerIPC, this));

  InitDevice();

  ezClock::SetNumGlobalClocks();

  SendReflectionInformation();

  SendProjectReadyMessage();
}

void ezEditorProcessApp::BeforeEngineShutdown()
{
  ezTelemetry::CloseConnection();

  m_IPC.m_Events.RemoveEventHandler(ezMakeDelegate(&ezEditorProcessApp::EventHandlerIPC, this));

  delete m_pApp;
}

ezApplication::ApplicationExecution ezEditorProcessApp::Run()
{
  m_IPC.ProcessMessages();

  if (!m_IPC.IsHostAlive())
    return ezApplication::Quit;

  ezTelemetry::PerFrameUpdate();

  ezThreadUtils::Sleep(1);
  return ezApplication::Continue;
}



