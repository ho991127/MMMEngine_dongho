#pragma once
#include <Windows.h>
#include <string>

#include "Delegates.hpp"

namespace MMMEngine::Utility
{
	/// <summary>
	/// 윈도우 하나의 대한 핸들과 루프를 제공합니다.
	/// </summary>
	class App
	{
	public:
		struct WindowInfo
		{
			std::wstring title;
			LONG width;
			LONG height;
			DWORD style;
		};

		struct WindowedRestore
		{
			DWORD style = 0;
			DWORD exStyle = 0;
			RECT  rect = { 0,0,0,0 };
			bool  valid = false;
		};

		App();
		App(HINSTANCE hInstance);
		App(LPCWSTR title, LONG width, LONG height);
		App(HINSTANCE hInstance,LPCWSTR title, LONG width, LONG height);

		~App();

		int Run();
		void Quit();

		void SetWindowTitle(const std::wstring& title);

		Event<App, void(void)> OnInitialize{ this };
		Event<App, void(void)> OnRelease{ this };
		Event<App, void(void)> OnUpdate{ this };
		Event<App, void(void)> OnRender{ this };
		Event<App, void(int,int)> OnWindowSizeChanged{ this };
		Event<App, void(HWND, UINT, WPARAM, LPARAM)> OnBeforeWindowMessage{ this };
		Event<App, void(HWND, UINT, WPARAM, LPARAM)> OnAfterWindowMessage{ this };

		const WindowInfo GetWindowInfo() const;
		HWND GetWindowHandle() const;

		void SetWindowSize(int width, int height);
	protected:
		LRESULT HandleWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	private:
		bool m_isRunning;
		WindowInfo m_windowInfo;
		
		HINSTANCE m_hInstance;
		HWND m_hWnd;

		bool m_windowSizeDirty;

		bool CreateMainWindow();
		static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	};
}