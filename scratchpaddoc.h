#pragma once

#include "mathlib/mathlib.h"
#include "scratchpad3d.h"
#include "ScratchPadUtils.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imesh.h"
#include "materialsystem/MaterialSystemUtil.h"
#include "KeyValues.h"

#include <functional>

extern IFileSystem* g_pFileSystem;

class ScratchPadDocument
{
public:

	~ScratchPadDocument()
	{
		Unlit.Shutdown();
		delete ScratchPad;
	}

	ScratchPadDocument(const char* filename)
	{
		ScratchPad = new CScratchPad3D(filename, g_pFileSystem, false);
	}

	bool Init()
	{
		Unlit.Init("ScratchPad_Unlit", new KeyValues("UnlitGeneric", "$vertexcolor", "1"));
		return ScratchPad->LoadCommandsFromFile();
	}

	void Draw(IMatRenderContext* pRenderContext)
	{
		int iLastCmd = -1;

		for (auto cmd : ScratchPad->m_Commands)
		{
			if (cmd->m_iCommand >= 0 && cmd->m_iCommand < CScratchPad3D::COMMAND_NUMCOMMANDS)
			{
				// Stop and End functions can be used to batch commands into a single draw
				if (cmd->m_iCommand != iLastCmd)
				{
					if (iLastCmd != -1)
					{
						if (CommandRenderFunctions[iLastCmd].m_StopFn)
							CommandRenderFunctions[iLastCmd].m_StopFn(pRenderContext);
					}

					iLastCmd = cmd->m_iCommand;

					if (CommandRenderFunctions[cmd->m_iCommand].m_StartFn)
						CommandRenderFunctions[cmd->m_iCommand].m_StartFn(pRenderContext);
				}

				CommandRenderFunctions[cmd->m_iCommand].m_RenderFn(cmd, pRenderContext);
			}
		}

		if (iLastCmd != -1)
		{
			if (CommandRenderFunctions[iLastCmd].m_StopFn)
				CommandRenderFunctions[iLastCmd].m_StopFn(pRenderContext);
		}
	}

private:

	void Command_RenderPoint(CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)
	{
	}

	void Command_Line_Start(IMatRenderContext* pRenderContext)
	{
	}
	void Command_Line_End(IMatRenderContext* pRenderContext)
	{
	}

	void Command_RenderLine(CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)
	{
	}

	void Command_RenderPolygon(CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)
	{
		CScratchPad3D::CCommand_Polygon* cmd = (CScratchPad3D::CCommand_Polygon*)pInCmd;

		pRenderContext->Bind(Unlit);
		IMesh* mesh = pRenderContext->GetDynamicMesh();
		CMeshBuilder meshBuilder;
		int nVerts = min(64, cmd->m_Verts.Size());
		meshBuilder.Begin(mesh, MATERIAL_POLYGON, nVerts);


		for (auto vert : cmd->m_Verts)
		{

			Vector col = vert.m_vColor.m_vColor;
			unsigned char r = LinearToGamma(col.x) * 255.0f, g = LinearToGamma(col.y) * 255.0f, b = LinearToGamma(col.z) * 255.0f;

			meshBuilder.Color4ub(r, g, b, 255);
			meshBuilder.TexCoord2f(0, 0, 0);
			meshBuilder.Position3fv(vert.m_vPos.Base());
			meshBuilder.AdvanceVertex();
		}

		meshBuilder.End();
		mesh->Draw();
	}

	void Command_SetMatrix(CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)
	{
	}

	void Command_SetRenderState(CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)
	{
	}

	void Command_RenderText(CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)
	{
	}

	CMaterialReference Unlit;

	CScratchPad3D *ScratchPad = nullptr;

	using CommandRenderFunction_Start = std::function<void(IMatRenderContext* pRenderContext)>;
	using CommandRenderFunction_Stop = std::function<void(IMatRenderContext* pRenderContext)>;
	using CommandRenderFunction = std::function<void(CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)>;

	struct CCommandRenderFunctions
	{
		CommandRenderFunction_Start m_StartFn;
		CommandRenderFunction_Stop m_StopFn;
		CommandRenderFunction m_RenderFn;
	};

	CCommandRenderFunctions CommandRenderFunctions[CScratchPad3D::COMMAND_NUMCOMMANDS] =
	{
		{ NULL, NULL,(CommandRenderFunction)[&](CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)->void { Command_RenderPoint(pInCmd, pRenderContext); }},
		{ 
			[&](IMatRenderContext* pRenderContext)->void { Command_Line_Start(pRenderContext); },
			[&](IMatRenderContext* pRenderContext)->void { Command_Line_End(pRenderContext); },
			[&](CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)->void { Command_RenderLine(pInCmd, pRenderContext); } 
		},
		{ NULL, NULL, [&](CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)->void { Command_RenderPolygon(pInCmd, pRenderContext); } },
		{ NULL, NULL, [&](CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)->void { Command_SetMatrix(pInCmd, pRenderContext); } },
		{ NULL, NULL, [&](CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)->void { Command_SetRenderState(pInCmd, pRenderContext); } },
		{ NULL, NULL, [&](CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)->void { Command_RenderText(pInCmd, pRenderContext); } }
	};
};
