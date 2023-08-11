#pragma once

#include "mathlib/mathlib.h"
#include "scratchpad3d.h"
#include "ScratchPadUtils.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imesh.h"

class ScratchPadDocument
{
public:

	~ScratchPadDocument()
	{
		delete ScratchPad;
	}

	ScratchPadDocument(const char* filename)
	{
		ScratchPad = new CScratchPad3D(filename, g_pFullFileSystem, false);
	}

	bool Init()
	{
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

	void Command_Line_Start(CScratchPad3D::CBaseCommand* pInCmd)
	{
	}
	void Command_Line_End(CScratchPad3D::CBaseCommand* pInCmd)
	{
	}

	void Command_RenderLine(CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)
	{
	}

	void Command_RenderPolygon(CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)
	{
		CScratchPad3D::CCommand_Polygon* cmd = (CScratchPad3D::CCommand_Polygon*)pInCmd;

		IMesh* mesh = pRenderContext->GetDynamicMesh();
		CMeshBuilder meshBuilder;
		int nVerts = min(64, cmd->m_Verts.Size());
		meshBuilder.Begin(mesh, MATERIAL_POLYGON, nVerts);

		for (auto vert : cmd->m_Verts)
		{
			meshBuilder.Position3fv(vert.m_vPos.Base());
			meshBuilder.Color4fv((float*)&vert.m_vColor);
		}

		meshBuilder.End(false, true);
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

	CScratchPad3D *ScratchPad = nullptr;

	using CommandRenderFunction_Start = void (*)(IMatRenderContext* pRenderContext);
	using CommandRenderFunction_Stop = void (*)(IMatRenderContext* pRenderContext);
	using CommandRenderFunction = void (*)(CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext);

	struct CCommandRenderFunctions
	{
		CommandRenderFunction_Start m_StartFn;
		CommandRenderFunction_Stop m_StopFn;
		CommandRenderFunction m_RenderFn;
	};

	CCommandRenderFunctions CommandRenderFunctions[CScratchPad3D::COMMAND_NUMCOMMANDS] =
	{
		{ NULL, NULL,(CommandRenderFunction)&[&](CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)->void { Command_RenderPoint(pInCmd, pRenderContext); }},
		{ 
			(CommandRenderFunction_Start)& [&](CScratchPad3D::CBaseCommand* pInCmd)->void { Command_Line_Start(pInCmd); },
			(CommandRenderFunction_Stop)& [&](CScratchPad3D::CBaseCommand* pInCmd)->void { Command_Line_End(pInCmd); },
			(CommandRenderFunction) & [&](CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)->void { Command_RenderLine(pInCmd, pRenderContext); } 
		},
		{ NULL, NULL, (CommandRenderFunction) & [&](CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)->void { Command_RenderPolygon(pInCmd, pRenderContext); } },
		{ NULL, NULL, (CommandRenderFunction) & [&](CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)->void { Command_SetMatrix(pInCmd, pRenderContext); } },
		{ NULL, NULL, (CommandRenderFunction) & [&](CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)->void { Command_SetRenderState(pInCmd, pRenderContext); } },
		{ NULL, NULL, (CommandRenderFunction) & [&](CScratchPad3D::CBaseCommand* pInCmd, IMatRenderContext* pRenderContext)->void { Command_RenderText(pInCmd, pRenderContext); } }
	};
};
