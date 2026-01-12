#pragma once
#include "GlobalRegistry.h"
//#include "RenderManager.h"

#include <cassert>

namespace MMMEngine::Screen
{
	inline void SetResolution(int width, int height) { assert(g_pApp && "글로벌 레지스트리에 Application이 등록되어있지 않습니다!");  g_pApp->SetWindowSize(width, height); }
}
