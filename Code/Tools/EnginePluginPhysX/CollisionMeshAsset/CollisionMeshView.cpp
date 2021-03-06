#include <EnginePluginPhysXPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/Component.h>
#include <Core/World/GameObject.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <EnginePluginPhysX/CollisionMeshAsset/CollisionMeshContext.h>
#include <EnginePluginPhysX/CollisionMeshAsset/CollisionMeshView.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

ezCollisionMeshViewContext::ezCollisionMeshViewContext(ezCollisionMeshContext* pMeshContext)
    : ezEngineProcessViewContext(pMeshContext)
{
  m_pMeshContext = pMeshContext;

  // Start with something valid.
  m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(ezVec3(1, 1, 1), ezVec3::ZeroVector(), ezVec3(0.0f, 0.0f, 1.0f));
}

ezCollisionMeshViewContext::~ezCollisionMeshViewContext() {}

bool ezCollisionMeshViewContext::UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -ezVec3(5, -2, 3));
}


ezViewHandle ezCollisionMeshViewContext::CreateView()
{
  ezView* pView = nullptr;
  ezRenderWorld::CreateView("Collision Mesh Editor - View", pView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void ezCollisionMeshViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  ezEngineProcessViewContext::SetCamera(pMsg);

  const ezUInt32 viewHeight = pMsg->m_uiWindowHeight;

  ezBoundingBox bbox;
  bbox.SetCenterAndHalfExtents(ezVec3::ZeroVector(), ezVec3::ZeroVector());

  auto hResource = m_pMeshContext->GetMesh();
  if (hResource.IsValid())
  {
    ezResourceLock<ezPxMeshResource> pResource(hResource, ezResourceAcquireMode::AllowFallback);
    bbox = pResource->GetBounds().GetBox();

    ezStringBuilder sText;
    sText.PrependFormat("Bounding Box: width={0}, depth={1}, height={2}", ezArgF(bbox.GetHalfExtents().x * 2, 2),
                        ezArgF(bbox.GetHalfExtents().y * 2, 2), ezArgF(bbox.GetHalfExtents().z * 2, 2));

    ezDebugRenderer::Draw2DText(m_hView, sText, ezVec2I32(10, viewHeight - 26), ezColor::White);
  }
}
