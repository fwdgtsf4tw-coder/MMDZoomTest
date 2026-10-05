#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <cwchar>\n#include <cmath>\n#include <algorithm>

#pragma comment(lib, "comctl32.lib")

namespace {
constexpr int IDC_FOV_SLIDER = 1001;
constexpr int IDC_FOV_VALUE  = 1002;
constexpr int IDC_STATUS     = 1003;
constexpr int MMD_FOV_ID     = 448;
constexpr int FOV_MIN        = 1;
constexpr int FOV_MAX        = 125;\nconstexpr int ZOOM_MIN_TENTHS = 10;\nconstexpr int ZOOM_MAX_TENTHS = 100;\nconstexpr double REF_FOV_DEG = 45.0;

HWND g_slider=nullptr, g_value=nullptr, g_status=nullptr;
struct FindCtx { HWND edit=nullptr; };

BOOL CALLBACK FindFovProc(HWND hwnd, LPARAM lp) {
    auto* ctx=reinterpret_cast<FindCtx*>(lp);
    if (GetDlgCtrlID(hwnd)!=MMD_FOV_ID) return TRUE;
    wchar_t cls[64]{};
    GetClassNameW(hwnd,cls,64);
    if (_wcsicmp(cls,L"Edit")==0) { ctx->edit=hwnd; return FALSE; }
    return TRUE;
}
HWND FindMmdWindow() {
    struct Ctx { HWND found=nullptr; } ctx;
    EnumWindows([](HWND hwnd, LPARAM lp)->BOOL {
        auto* c=reinterpret_cast<Ctx*>(lp);
        if (!IsWindowVisible(hwnd)) return TRUE;
        wchar_t title[512]{};
        GetWindowTextW(hwnd,title,512);
        if (wcsstr(title,L"MikuMikuDance") || wcsstr(title,L"MikuMikuDance.exe")) {
            FindCtx fc;
            EnumChildWindows(hwnd,FindFovProc,reinterpret_cast<LPARAM>(&fc));
            if (fc.edit) { c->found=hwnd; return FALSE; }
        }
        return TRUE;
    },reinterpret_cast<LPARAM>(&ctx));
    return ctx.found;
}
HWND FindFovEdit(HWND mmd) {
    FindCtx ctx;
    EnumChildWindows(mmd,FindFovProc,reinterpret_cast<LPARAM>(&ctx));
    return ctx.edit;
}
bool ApplyFov(int fov) {
    HWND mmd=FindMmdWindow();
    if (!mmd) { SetWindowTextW(g_status,L"MMD 9.32 FOV control not found"); return false; }
    HWND edit=FindFovEdit(mmd);
    if (!edit) { SetWindowTextW(g_status,L"FOV control (ID 448) not found"); return false; }
    wchar_t buf[16]{};
    swprintf_s(buf,L"%d",fov);
    if (!SendMessageW(edit,WM_SETTEXT,0,reinterpret_cast<LPARAM>(buf))) {
        SetWindowTextW(g_status,L"Failed to write FOV"); return false;
    }
    SendMessageW(edit,WM_KEYDOWN,VK_RETURN,0);
    SetWindowTextW(g_status,L"FOV sent to MMD (no camera key registered)");
    return true;
}
void UpdateValue(int fov) {
    wchar_t buf[32]{};
    swprintf_s(buf,L"FOV : %d deg",fov);
    SetWindowTextW(g_value,buf);
}
LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    switch(msg) {
    case WM_CREATE:
        CreateWindowW(L"STATIC",L"MMD Zoom Test - ZOOM to FOV",WS_CHILD|WS_VISIBLE,18,15,300,24,hwnd,nullptr,nullptr,nullptr);
        g_slider=CreateWindowW(TRACKBAR_CLASSW,L"",WS_CHILD|WS_VISIBLE|TBS_AUTOTICKS,18,48,330,45,hwnd,reinterpret_cast<HMENU>(IDC_FOV_SLIDER),nullptr,nullptr);
        SendMessageW(g_slider,TBM_SETRANGE,TRUE,MAKELPARAM(ZOOM_MIN_TENTHS,ZOOM_MAX_TENTHS));
        SendMessageW(g_slider,TBM_SETTICFREQ,10,0);
        SendMessageW(g_slider,TBM_SETPOS,TRUE,10);
        g_value=CreateWindowW(L"STATIC",L"ZOOM : 1.0x    FOV : 45 deg",WS_CHILD|WS_VISIBLE,18,100,180,24,hwnd,reinterpret_cast<HMENU>(IDC_FOV_VALUE),nullptr,nullptr);
        g_status=CreateWindowW(L"STATIC",L"ZOOM -> FOV test. No camera key is registered.",WS_CHILD|WS_VISIBLE,18,132,355,44,hwnd,reinterpret_cast<HMENU>(IDC_STATUS),nullptr,nullptr);
        return 0;
    case WM_HSCROLL:
        if (reinterpret_cast<HWND>(lp)==g_slider) {
            int fov=static_cast<int>(SendMessageW(g_slider,TBM_GETPOS,0,0));
            UpdateValue(fov); ApplyFov(fov);
        }
        return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}
}
int WINAPI wWinMain(HINSTANCE inst,HINSTANCE,PWSTR,int show) {
    INITCOMMONCONTROLSEX icc{sizeof(icc),ICC_BAR_CLASSES}; InitCommonControlsEx(&icc);
    const wchar_t* kClass=L"MMDZoomTestWindow";
    WNDCLASSW wc{}; wc.lpfnWndProc=WndProc; wc.hInstance=inst; wc.lpszClassName=kClass;
    wc.hCursor=LoadCursor(nullptr,IDC_ARROW); wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
    if(!RegisterClassW(&wc)) return 1;
    HWND hwnd=CreateWindowExW(0,kClass,L"MMD Zoom Test 0.2",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
        CW_USEDEFAULT,CW_USEDEFAULT,400,225,nullptr,nullptr,inst,nullptr);
    if(!hwnd) return 2;
    ShowWindow(hwnd,show); UpdateWindow(hwnd);
    MSG msg{}; while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
    return static_cast<int>(msg.wParam);
}