#include "App.h"
#include <wrl/client.h>

static RECT GetMonitorRectForWindow(HWND hwnd)
{
	HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi{ sizeof(mi) };
	GetMonitorInfo(mon, &mi);
	return mi.rcMonitor;
}

static bool IsBorderless(HWND hwnd)
{
	LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
	// 일반적으로 borderless는 WS_POPUP 기반
	if (style & WS_CAPTION) return false;
	if (style & WS_THICKFRAME) return false;

	RECT wr{}, mr{};
	GetWindowRect(hwnd, &wr);
	mr = GetMonitorRectForWindow(hwnd);

	return (wr.left == mr.left && wr.top == mr.top &&
		wr.right == mr.right && wr.bottom == mr.bottom);
}

MMMEngine::Utility::App::App()
	: m_hInstance(GetModuleHandle(NULL))
	, m_hWnd(NULL)
	, m_isRunning(false)
	, m_windowSizeDirty(false)
	, m_windowInfo({ L"MMM Engine Application",1600,900,WS_OVERLAPPEDWINDOW })
{
}

MMMEngine::Utility::App::App(HINSTANCE hInstance)
	: m_hInstance(hInstance)
	, m_hWnd(NULL)
	, m_isRunning(false)
	, m_windowSizeDirty(false)
	, m_windowInfo({ L"MMM Engine Application",1600,900,WS_OVERLAPPEDWINDOW })
{
}

MMMEngine::Utility::App::App(LPCWSTR title, LONG width, LONG height)
	: m_hInstance(GetModuleHandle(NULL))
	, m_hWnd(NULL)
	, m_isRunning(false)
	, m_windowSizeDirty(false)
	, m_windowInfo({ title,width,height,WS_OVERLAPPEDWINDOW })
{
}

MMMEngine::Utility::App::App(HINSTANCE hInstance, LPCWSTR title, LONG width, LONG height)
	: m_hInstance(hInstance)
	, m_hWnd(NULL)
	, m_isRunning(false)
	, m_windowSizeDirty(false)
	, m_windowInfo({ title,width,height,WS_OVERLAPPEDWINDOW })
{
}

MMMEngine::Utility::App::~App()
{

}

int MMMEngine::Utility::App::Run()
{
	if (FAILED(CoInitialize(nullptr)))
		return -1;

	if (!CreateMainWindow())
		return -2;

	m_isRunning = true;
	OnInitialize(this);
	MSG msg = {};
	while (m_isRunning && msg.message != WM_QUIT)
	{
		if (m_windowSizeDirty)
		{
			OnWindowSizeChanged(this, m_windowInfo.width, m_windowInfo.height);
			m_windowSizeDirty = false;
		}

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			OnUpdate(this);
			OnRender(this);
		}
	}
	OnRelease(this);

	CoUninitialize();
	return (int)msg.wParam;
}

void MMMEngine::Utility::App::Quit()
{
	m_isRunning = false;
	if (m_hWnd)
		PostMessageW(m_hWnd, WM_CLOSE, 0, 0);
	else
		PostQuitMessage(0);
}

const MMMEngine::Utility::App::WindowInfo MMMEngine::Utility::App::GetWindowInfo() const
{
	return m_windowInfo;
}

HWND MMMEngine::Utility::App::GetWindowHandle() const
{
	return m_hWnd;
}

void MMMEngine::Utility::App::SetWindowSize(int width, int height)
{
	if (!m_hWnd)
	{
		return;
	}

	RECT windowRect = { 0, 0, width, height };

	BOOL success = ::AdjustWindowRect(&windowRect, m_windowInfo.style, FALSE);

	if (success)
	{
		LONG adjustedWidth = windowRect.right - windowRect.left;
		LONG adjustedHeight = windowRect.bottom - windowRect.top;

		::SetWindowPos(
			m_hWnd,
			NULL,               // Z-Order 변경 안 함
			0, 0,               // SWP_NOMOVE 플래그를 사용하므로 X, Y는 무시됨
			adjustedWidth,      // 계산된 전체 폭
			adjustedHeight,     // 계산된 전체 높이
			SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED
		);
	}
	
}

LRESULT MMMEngine::Utility::App::HandleWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	OnBeforeWindowMessage(this, hWnd, uMsg, wParam, lParam);

	LRESULT result = 0;

	switch (uMsg) {
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED &&
			(wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)) {

			RECT clientRect;
			GetClientRect(hWnd, &clientRect);
			LONG newWidth = clientRect.right - clientRect.left;
			LONG newHeight = clientRect.bottom - clientRect.top;

			// 리사이즈 로직을 D3DContext에 위임
			if (newWidth != m_windowInfo.width || newHeight != m_windowInfo.height) {
				m_windowInfo.width = newWidth;
				m_windowInfo.height = newHeight;
				m_windowSizeDirty = true;
			}
		}
		break;
	default:
		result = DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;
	}

	if (result == 0)
	{
		result = DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	OnAfterWindowMessage(this, hWnd, uMsg, wParam, lParam);

	return result;
}

bool MMMEngine::Utility::App::CreateMainWindow()
{
	// 윈도우 클래스 정의
	WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = m_hInstance;
	wc.lpszClassName = L"MMMEngineClass";
	ATOM a = RegisterClassExW(&wc);
	if (a == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;

	// 클라이언트 영역에 맞춰 윈도우 전체 크기 계산
	RECT clientRect = { 0, 0, m_windowInfo.width, m_windowInfo.height };
	AdjustWindowRect(&clientRect, WS_OVERLAPPEDWINDOW, FALSE);

	// 윈도우 생성
	m_hWnd = CreateWindowEx(
		0,
		wc.lpszClassName,
		m_windowInfo.title.c_str(),
		m_windowInfo.style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		clientRect.right - clientRect.left,
		clientRect.bottom - clientRect.top,
		NULL,
		NULL,
		m_hInstance,
		this
	);
	if (!m_hWnd) return false;

	ShowWindow(m_hWnd, SW_SHOW);
	return true;
}

void MMMEngine::Utility::App::SetWindowTitle(const std::wstring& title)
{
	m_windowInfo.title = title;

	// 윈도우가 이미 생성되어 있으면 즉시 제목 변경
	if (m_hWnd)
	{
		SetWindowTextW(m_hWnd, m_windowInfo.title.c_str());
	}
}


LRESULT MMMEngine::Utility::App::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_NCCREATE) {
		auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	App* pApp = reinterpret_cast<App*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if (pApp) {
		return pApp->HandleWindowMessage(hWnd, uMsg, wParam, lParam);
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}