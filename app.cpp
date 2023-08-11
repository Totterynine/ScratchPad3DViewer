#include "app.h"
#include "materialsystem/imaterialproxyfactory.h"
#include "materialsystem/ITexture.h"
#include "materialsystem/MaterialSystem_Config.h"
#include "istudiorender.h"

#include "studiomodel.h"

// Bring in our non-source things
#include "memdbgoff.h"

#include <imgui_impl_source.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/imgui.h>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32 1
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "scratchpaddoc.h"

// Currently blank, but might be worth filling in if you need mat proxies
class CDummyMaterialProxyFactory : public IMaterialProxyFactory
{
public:
	virtual IMaterialProxy *CreateProxy(const char *proxyName) { return nullptr; }
	virtual void DeleteProxy(IMaterialProxy *pProxy) { }
};
CDummyMaterialProxyFactory g_DummyMaterialProxyFactory;

void* ImGui_MemAlloc(size_t sz, void* user_data)
{
	return MemAlloc_Alloc(sz, "ImGui", 0);
}

static void ImGui_MemFree(void* ptr, void* user_data)
{
	MemAlloc_Free(ptr);
}

void CScratchPad3DViewer::Init()
{
	if (!glfwInit())
		return;

	// We're handled by matsys, no api
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	m_pWindow = glfwCreateWindow(1280, 720, "ScrachPad3D Viewer", NULL, NULL);
	if (!m_pWindow)
		return;

	glfwMakeContextCurrent(m_pWindow);
#ifdef _WIN32
	HWND hwnd = glfwGetWin32Window(m_pWindow);
#endif

	// Set up matsys
	MaterialSystem_Config_t config;
	config = g_pMaterialSystem->GetCurrentConfigForVideoCard();
	config.SetFlag(MATSYS_VIDCFG_FLAGS_WINDOWED, true);
	config.SetFlag(MATSYS_VIDCFG_FLAGS_RESIZING, true);

	// Feed material system our window
	if (!g_pMaterialSystem->SetMode((void*)hwnd, config))
		return;
	g_pMaterialSystem->OverrideConfig(config, false);

	// We want to set this before we load up any mats, else it'll reload em all
	g_pMaterialSystem->SetMaterialProxyFactory(&g_DummyMaterialProxyFactory);

	// White out our cubemap and lightmap, as we don't have either
	m_pWhiteTexture = g_pMaterialSystem->FindTexture("white", NULL, true);
	m_pWhiteTexture->AddRef();
	g_pMaterialSystem->GetRenderContext()->BindLocalCubemap(m_pWhiteTexture);
	g_pMaterialSystem->GetRenderContext()->BindLightmapTexture(m_pWhiteTexture);

	// If we don't do this, all models will render black
	int samples = g_pStudioRender->GetNumAmbientLightSamples();
	m_ambientLightColors = new Vector[samples];
	for (int i = 0; i < samples; i++)
		m_ambientLightColors[i] = { 1,1,1 };
	g_pStudioRender->SetAmbientLightColors(m_ambientLightColors);
	
	// Init Dear ImGui
	ImGui::SetAllocatorFunctions(ImGui_MemAlloc, ImGui_MemFree, nullptr);
	ImGui::CreateContext();
	ImGui_ImplSource_Init();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOther(m_pWindow, true);
//	ImGui_ImplSourceGLFW_Init(m_pWindow, (void*)hwnd);

	m_lastFrameTime = glfwGetTime();

	// Main app loop
	while (!glfwWindowShouldClose(m_pWindow))
	{
		glfwPollEvents();
		ImGui_ImplGlfw_NewFrame();
		DrawFrame();
	}
}

void CScratchPad3DViewer::Destroy()
{
	ImGui_ImplSource_Shutdown();
	ImGui_ImplGlfw_Shutdown();

	// Clean up all of our assets, windows, etc
	if(m_ambientLightColors)
		delete[] m_ambientLightColors;

	if (m_pWhiteTexture)
		m_pWhiteTexture->DecrementReferenceCount();

	if(m_pWindow)
		glfwDestroyWindow(m_pWindow);
	glfwTerminate();
}

void CScratchPad3DViewer::DrawFrame()
{
	// What's our delta time?
	float curTime = glfwGetTime();
	float dt = curTime - m_lastFrameTime;
	m_lastFrameTime = curTime;

	// Start Frame
	g_pMaterialSystem->BeginFrame(0);

	// Clear out the old frame
	CMatRenderContextPtr ctx(g_pMaterialSystem);
	ctx->ClearColor3ub(0x30, 0x30, 0x30);
	ctx->ClearBuffers(true, true);

	// Let it know our window size
	int w, h;
	glfwGetWindowSize(m_pWindow, &w, &h);
	ctx->Viewport(0, 0, w, h);

	// Begin ImGui
	// Ideally this happens before we branch off into other functions, as it needs to be setup for other sections of code to use imgui
	ImGuiIO& io = ImGui::GetIO();
	ImGui::NewFrame();

	// Make us a nice camera
	VMatrix viewMatrix;
	VMatrix projMatrix;
	ComputeViewMatrix(&viewMatrix, View);
	ComputeProjectionMatrix(&projMatrix, View, w, h);

	// 3D Rendering mode
	ctx->MatrixMode(MATERIAL_PROJECTION);
	ctx->LoadMatrix(projMatrix);
	ctx->MatrixMode(MATERIAL_VIEW);
	ctx->LoadMatrix(viewMatrix);

	// Mouse input
	// If we're dragging a window, we don't want to be dragging our model too
	if (!io.WantCaptureMouse)
	{
		// Slow down our zoom as we get closer in for finer movements
		float mw = io.MouseWheel;
		mw *= ViewZoom / 20.0f;
		
		// Don't allow zooming into and past our model
		ViewZoom += ViewZoom;
		if (ViewZoom <= 1)
			ViewZoom = 1;

		// Camera rotation
		float x = io.MousePos.x;
		float y = io.MousePos.y;
		static float ox = 0, oy = 0;
		if (io.MouseDown[0])
		{
			View.m_angles.y -= x - ox;
			View.m_angles.x += y - oy;

			Vector forward, right, up;
			AngleVectors(View.m_angles, &forward, &right, &up);

			float moveScale = ViewZoom / 10.0f;

			// Keyboard input
			if (ImGui::IsKeyDown(ImGuiKey_W))
			{
				View.m_origin += forward * moveScale;
			}
			if (ImGui::IsKeyDown(ImGuiKey_S))
			{
				View.m_origin -= forward * moveScale;
			}

			if (ImGui::IsKeyDown(ImGuiKey_A))
			{
				View.m_origin -= right * moveScale;
			}
			if (ImGui::IsKeyDown(ImGuiKey_D))
			{
				View.m_origin += right * moveScale;
			}
		}
		ox = x;
		oy = y;
	}

	if (ImGui::Begin("Document"))
	{
		static char s_PadFilename[MAX_PATH] = "scratch.pad";
		ImGui::InputText("Path", s_PadFilename, sizeof(s_PadFilename));
		ImGui::SameLine();
		if (ImGui::Button("Apply"))
		{
			if (Document)
				delete Document;

			Document = new ScratchPadDocument(s_PadFilename);

			if (!Document->Init())
			{
				Msg("Failed to load %s file\n", s_PadFilename);

				delete Document;
				Document = nullptr;
			}
		}
	}
	ImGui::End();

	// End ImGui, and let it draw
	ImGui::Render();

	CMatRenderContextPtr pRenderContext(g_pMaterialSystem);

	if (Document)
		Document->Draw(pRenderContext);

	ImGui_ImplSource_RenderDrawData(ImGui::GetDrawData());

	// End Frame
	g_pMaterialSystem->SwapBuffers();
	g_pMaterialSystem->EndFrame();
}
