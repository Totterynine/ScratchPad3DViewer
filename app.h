#pragma once

#include "tier2/camerautils.h"

struct GLFWwindow;
class ITexture;
class Vector;
class ScratchPadDocument;

// I would just merge this into CSteamAppLoader, but it makes such a mess
class CScratchPad3DViewer
{
public:
	void Init();
	void Destroy();
private:

	void DrawFrame();

	float ViewZoom = 120.0f;
	Camera_t View = { {-ViewZoom, 0, 0}, {0, 0, 0}, 65, 1.0f, 20000.0f };

	ScratchPadDocument* Document = nullptr;

	GLFWwindow* m_pWindow;
	ITexture *m_pWhiteTexture;

	float m_lastFrameTime;

	Vector* m_ambientLightColors;
};