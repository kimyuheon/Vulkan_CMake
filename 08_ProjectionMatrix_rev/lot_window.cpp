#include "lot_window.h"
#include <stdexcept>

#ifdef _WIN32
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3native.h>
    #include <windows.h>

    static WNDPROC g_OldProc = nullptr;
    static HBRUSH  g_ClearBrush = nullptr;
    static HBITMAP g_LastFrameBitmap = nullptr;
    static HDC     g_LastFrameDC = nullptr;
    static int     g_LastFrameWidth = 0;
    static int     g_LastFrameHeight = 0;
    static constexpr COLORREF kClearRGB = RGB(3,3,3);

    // 현재 창 내용을 비트맵으로 캡처
    void CaptureLastFrame(HWND hwnd) {
        HDC windowDC = GetDC(hwnd);
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;

        if (width <= 0 || height <= 0) {
            ReleaseDC(hwnd, windowDC);
            return;
        }

        // 기존 리소스 정리
        if (g_LastFrameBitmap) {
            DeleteObject(g_LastFrameBitmap);
            g_LastFrameBitmap = nullptr;
        }
        if (g_LastFrameDC) {
            DeleteDC(g_LastFrameDC);
            g_LastFrameDC = nullptr;
        }

        // 새 비트맵과 DC 생성
        g_LastFrameDC = CreateCompatibleDC(windowDC);
        g_LastFrameBitmap = CreateCompatibleBitmap(windowDC, width, height);
        SelectObject(g_LastFrameDC, g_LastFrameBitmap);

        // 현재 창 내용을 비트맵으로 복사
        BitBlt(g_LastFrameDC, 0, 0, width, height, windowDC, 0, 0, SRCCOPY);

        // 원본 크기 저장
        g_LastFrameWidth = width;
        g_LastFrameHeight = height;

        ReleaseDC(hwnd, windowDC);
    }

    // 저장된 마지막 프레임을 빠르게 그리기
    void DrawLastFrameFast(HWND hwnd, HDC dc) {
        if (!g_LastFrameDC || !g_LastFrameBitmap) {
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            if (g_ClearBrush) {
                FillRect(dc, &clientRect, g_ClearBrush);
            }
            return;
        }

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;

        // 빠른 스트레치 모드
        SetStretchBltMode(dc, COLORONCOLOR);
        SetBkMode(dc, OPAQUE);

        // 즉시 스트레치
        StretchBlt(dc, 0, 0, width, height, 
                   g_LastFrameDC, 0, 0, g_LastFrameWidth, g_LastFrameHeight, SRCCOPY);
    }

    static LRESULT CALLBACK MyWndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
        auto* self = reinterpret_cast<lot::LotWindow*>(GetWindowLongPtr(h, GWLP_USERDATA));

        switch (m) {
            case WM_ENTERSIZEMOVE:
                if (self) {
                    CaptureLastFrame(h);
                    self->setUserResizing(true);
                }
                return 0;

            case WM_SIZING:
                // 즉시 응답
                if (g_LastFrameDC && g_LastFrameBitmap) {
                    HDC dc = GetDC(h);
                    DrawLastFrameFast(h, dc);
                    ReleaseDC(h, dc);
                } else if (g_ClearBrush) {
                    HDC dc = GetDC(h);
                    RECT rect;
                    GetClientRect(h, &rect);
                    FillRect(dc, &rect, g_ClearBrush);
                    ReleaseDC(h, dc);
                }
                ValidateRect(h, nullptr);
                return 0;

            case WM_SIZE:
                if (self && self->isUserResizing()) {
                    if (g_LastFrameDC && g_LastFrameBitmap) {
                        HDC dc = GetDC(h);
                        DrawLastFrameFast(h, dc);
                        ReleaseDC(h, dc);
                    }
                    ValidateRect(h, nullptr);
                    return 0;
                }
                break;

            case WM_EXITSIZEMOVE:
                if (self) {
                    self->setUserResizing(false);
                    // 리소스 정리
                    if (g_LastFrameBitmap) {
                        DeleteObject(g_LastFrameBitmap);
                        g_LastFrameBitmap = nullptr;
                    }
                    if (g_LastFrameDC) {
                        DeleteDC(g_LastFrameDC);
                        g_LastFrameDC = nullptr;
                    }
                    g_LastFrameWidth = 0;
                    g_LastFrameHeight = 0;
                }
                return 0;

            case WM_ERASEBKGND:
                return 1;

            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC dc = BeginPaint(h, &ps);
                
                if (self && self->isUserResizing()) {
                    DrawLastFrameFast(h, dc);
                } else if (g_ClearBrush) {
                    FillRect(dc, &ps.rcPaint, g_ClearBrush);
                }
                
                EndPaint(h, &ps);
                return 0;
            }
        }

        return CallWindowProc(g_OldProc, h, m, w, l);
    }
#endif

namespace lot {
    LotWindow::LotWindow(int w, int h, std::string name) 
    : width{w}, height{h}, windowName{name} {
        initWindow();
    }

    LotWindow::~LotWindow() {
        #ifdef _WIN32
            if (window && g_OldProc) {
                HWND hwnd = glfwGetWin32Window(window);
                SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_OldProc));
                g_OldProc = nullptr;
                SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, 0);
            }
            if (g_ClearBrush) {
                DeleteObject(g_ClearBrush);
                g_ClearBrush = nullptr;
            }
            if (g_LastFrameBitmap) {
                DeleteObject(g_LastFrameBitmap);
                g_LastFrameBitmap = nullptr;
            }
            if (g_LastFrameDC) {
                DeleteDC(g_LastFrameDC);
                g_LastFrameDC = nullptr;
            }
        #endif

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void LotWindow::initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

        #ifdef _WIN32
            HWND hwnd = glfwGetWin32Window(window);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

            // 서브클래싱
            g_OldProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(hwnd, GWLP_WNDPROC));
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(MyWndProc));

            // 빠른 리사이징을 위한 창 스타일 최적화
            LONG style = GetWindowLong(hwnd, GWL_STYLE);
            style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
            SetWindowLong(hwnd, GWL_STYLE, style);

            LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
            exStyle |= WS_EX_COMPOSITED;
            SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);

            // 배경 브러시 설정
            if (!g_ClearBrush) g_ClearBrush = CreateSolidBrush(kClearRGB);
            SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(g_ClearBrush));
        #endif
    }

    void LotWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surfacec_) {
        if (glfwCreateWindowSurface(instance, window, nullptr, surfacec_) != VK_SUCCESS)
            throw std::runtime_error("failed to ctreate window surface");
    }

    void LotWindow::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
        auto lotWindow = reinterpret_cast<LotWindow *>(glfwGetWindowUserPointer(window));
        lotWindow->framebufferResized = true;
        lotWindow->width = width;
        lotWindow->height = height;
    }
} // namespace lot