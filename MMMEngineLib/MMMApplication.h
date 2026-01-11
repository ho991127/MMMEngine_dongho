#pragma once
#include "GlobalRegistry.h"
#include <cassert>
#include <string>

namespace MMMEngine::Application
{
	inline void Quit() { assert(g_pApp && "글로벌 레지스트리에 Application이 등록되어있지 않습니다!"); g_pApp->Quit(); }

	inline void SetTitle(const std::wstring& title) { assert(g_pApp && "글로벌 레지스트리에 Application이 등록되어있지 않습니다!"); g_pApp->SetTitle(title.c_str()); }
}