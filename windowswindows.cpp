#include "windowswindows.h"
#include <iostream>
#include <WinUser.h>
#include <dwmapi.h>

extern WindowsWindows *m_windows;

WindowsWindows::WindowsWindows(QObject *parent) : QObject(parent)
{
    m_windows = this;

    CoInitializeEx(NULL,COINIT_MULTITHREADED);
    g_hook = SetWinEventHook(
                EVENT_SYSTEM_FOREGROUND, EVENT_OBJECT_LOCATIONCHANGE,
                NULL,                                          // Handle to DLL.
                &WindowsWindows::Wineventproc,                                // The callback.
                0, 0,              // Process and thread IDs of interest (0 = all)
                WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS); // Flags.
}

WindowsWindows::~WindowsWindows()
{
    if(!g_hook){
        UnhookWinEvent(g_hook);
        CoUninitialize();
    }
}

QMap<QString, QUrl> WindowsWindows::getWindows()
{
    QMap<QString, QUrl> windows;
    EnumWindows(&WindowsWindows::enumWindowCallback, reinterpret_cast<LPARAM>(&windows));
    return windows;
}

LRESULT WindowsWindows::CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    CHAR szCWPBuf[256];
    CHAR szMsg[16];
    HDC hdc;
    static int c = 0;
    size_t cch;
    HRESULT hResult;

    std::wcout << L"CallWndProc" << std::endl;

    if (nCode < 0)  // do not process message
        return CallNextHookEx(NULL, nCode, wParam, lParam);

    CWPSTRUCT *paramData = (LPCWPSTRUCT)lParam;

    if(paramData->message==WM_MOVE){
        int  xPos = (int)(short) LOWORD(paramData->lParam);   // horizontal position
        int yPos = (int)(short) HIWORD(paramData->lParam);   // vertical position
        std::wcout << "MOVE WINDOW x: " << xPos << "y: " << yPos << std::endl;
    }


    //        case WM_SIZE:


}

bool WindowsWindows::equalRect(const RECT &rect1, const RECT &rect2)
{
    return  rect1.bottom == rect2.bottom && rect1.left == rect2.left && rect1.right == rect2.right && rect1.top == rect2.top;
    //    return (compare_longs(rect1.bottom,rect2.bottom) && compare_longs(rect1.left,rect2.left) && compare_longs(rect1.right,rect2.right) && compare_longs(rect1.top,rect2.top));
}

bool WindowsWindows::equalRect2(const LONG *a, const LONG *b)
{
    __m128i xmm_a0 = _mm_loadu_si128((__m128i*)&a[0]);
    __m128i xmm_a1 = _mm_loadu_si128((__m128i*)&a[4]);
    __m128i xmm_b0 = _mm_loadu_si128((__m128i*)&b[0]);
    __m128i xmm_b1 = _mm_loadu_si128((__m128i*)&b[4]);
    __m128i xmm_result0 = _mm_cmpeq_epi64(xmm_a0, xmm_b0);
    __m128i xmm_result1 = _mm_cmpeq_epi64(xmm_a1, xmm_b1);
    __m128i xmm_result = _mm_packs_epi32(xmm_result0, xmm_result1);
    int result = _mm_movemask_epi8(xmm_result);
    return result == 0xFF;
}

std::wstring WindowsWindows::getWindowPath(HWND hWnd){
    DWORD pid;
    WORD err = GetWindowThreadProcessId(hWnd,&pid);
    HANDLE processHandle = NULL;
    processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ ,FALSE,pid);
    if(processHandle!=NULL){
        WCHAR path[512];
        GetModuleFileNameExW(processHandle,NULL,path,512);
        return std::wstring(path);
    }
    return L"";
}

void WindowsWindows::Wineventproc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime){
    if(!m_windows)
        return;
    if(event==EVENT_SYSTEM_FOREGROUND){
        WCHAR *Windowname;
        if(hwnd!=NULL){
            int length = GetWindowTextLengthW(hwnd);
            if(length==0)
                return;
            RECT r,frame;
            GetWindowRect(hwnd,&r);
            m_windows->currentRecct = r;
            m_windows->currentForeground = hwnd;
            GetUpdateRect(hwnd,&r,FALSE);

            DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(RECT));
            RECT border;
            border.left = frame.left - r.left;
            border.top = frame.top - r.top;
            border.right = r.right - frame.right;
            border.bottom = r.bottom - frame.bottom;
            QRect rect(r.left + border.left, r.top + border.top ,(r.right - r.left) - (border.left + border.right) , (r.bottom - r.top) - (border.top + border.bottom));
            Windowname = new wchar_t[length + 1];
            GetWindowTextW(hwnd,&Windowname[0],length+1);
            QUERY_USER_NOTIFICATION_STATE  state;
            HRESULT res = SHQueryUserNotificationState(&state);
            if(res!= S_OK){
                state = QUNS_NOT_PRESENT;
            }
            bool fullscreen = false;
            fullscreen = (state == QUNS_BUSY) || (state == QUNS_RUNNING_D3D_FULL_SCREEN) || (state == QUNS_PRESENTATION_MODE);
//            std::wcout << L"Window Name " << Windowname << L" Path " << getWindowPath(hwnd) << L" Visible " << IsWindowVisible(hwnd) << std::endl;
            m_windows->ForeGroundWindowChanged(QString::fromStdWString(getWindowPath(hwnd)),QString::fromWCharArray(Windowname,length),rect,fullscreen);
            delete[] Windowname;

            //******************

            //            if(hwnd!=m_windows->currentForeground){
            //                if(m_windows->currentForeground && m_windows->windowHook){
            //                    UnhookWindowsHookEx(m_windows->windowHook);
            //                }
            //            }
            //            HINSTANCE process = (HINSTANCE)GetWindowLongPtr(hwnd,GWLP_HINSTANCE);
            //            //            PVOID pfn = (IsWindowUnicode(hwnd) ? GetWindowLongPtrW : GetWindowLongPtrA)(hwnd, GWLP_HINSTANCE);
            //            DWORD processID;
            //            DWORD threadID =  GetWindowThreadProcessId(hwnd,&processID);
            //            HINSTANCE inst = GetModuleHandle(NULL);
            //            m_windows->windowHook = SetWindowsHookExW(WH_CALLWNDPROC,&WindowsWindows::CallWndProc,inst,threadID);
            //            if(m_windows->windowHook==NULL){
            //                std::wcout << "Error on hook " << GetLastError() << std::endl;
            //            }
            //            m_windows->currentForeground = hwnd;

        }
    }else if(event == EVENT_OBJECT_LOCATIONCHANGE && hwnd!=NULL && m_windows->currentForeground == hwnd){

        ///auto start = std::chrono::high_resolution_clock::now();
        RECT r,frame;
        GetWindowRect(hwnd,&r);

        if(r.left <= -31900 || r.bottom <=-31900)
            return;
        //        std::wcout << "Saved Rect " << m_windows->currentRecct.left << " " << m_windows->currentRecct.bottom << " " << m_windows->currentRecct.right<< " " << m_windows->currentRecct.top << std::endl;
        //        std::wcout << "NEW Rect   " << r.left << " " << r.bottom << " " << r.right << " " << r.top << std::endl;
//        LONG *lr = reinterpret_cast<LONG*>(&r);

//        std::wcout << "NEW Rect   " << r.left << " " << r.bottom << " " << r.right << " " << r.top << std::endl;
//        std::wcout << "NEW Rect   " << lr[0] << " " << lr[1] << " " << lr[2] << " " << lr[3] << std::endl;
///         auto start2 = std::chrono::high_resolution_clock::now();
//        if(equalRect2((LONG*)&m_windows->currentRecct,(LONG*)&r)){
//            return;
//        }

         if(equalRect(m_windows->currentRecct,r)){
             return;
         }
///        auto finish = std::chrono::high_resolution_clock::now();
///        std::wcout << L"Window R " << std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count() << "ns" << " Compare " << std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start2).count() << std::endl;

        ///start = std::chrono::high_resolution_clock::now();
        WCHAR *Windowname;
        int length = GetWindowTextLengthW(hwnd);
        if(length==0)
            return;
        Windowname = new wchar_t[length + 1];
        GetWindowTextW(hwnd,&Windowname[0],length+1);
        if(wcscmp(Windowname,L"Start")==0 && getWindowPath(hwnd)== LR"(C:\Windows\explorer.exe)"){
            delete[] Windowname;
            return;
        }
       /// finish = std::chrono::high_resolution_clock::now();
       /// std::wcout << L"Window N " << std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count() << "ns" << std::endl;

        GetUpdateRect(hwnd,&r,FALSE);
        DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(RECT));
        RECT border;
        border.left = frame.left - r.left;
        border.top = frame.top - r.top;
        border.right = r.right - frame.right;
        border.bottom = r.bottom - frame.bottom;
        QRect rect(r.left + border.left, r.top + border.top ,(r.right - r.left) - (border.left + border.right) , (r.bottom - r.top) - (border.top + border.bottom));
        QUERY_USER_NOTIFICATION_STATE  state;
        HRESULT res = SHQueryUserNotificationState(&state);
        if(res!= S_OK){
            state = QUNS_NOT_PRESENT;
        }

        bool fullscreen = false;
        fullscreen = (state == QUNS_BUSY) || (state == QUNS_RUNNING_D3D_FULL_SCREEN) || (state == QUNS_PRESENTATION_MODE);
//        std::wcout << L"MOVED Window Name " << Windowname << L" Path " << getWindowPath(hwnd) << std::endl;
        m_windows->ForeGroundWindowChanged(QString::fromStdWString(getWindowPath(hwnd)),QString::fromWCharArray(Windowname,length),rect,fullscreen);
    }
}

BOOL WindowsWindows::enumWindowCallback(HWND hWnd, LPARAM lparam) {
    if(!m_windows || !lparam)
        return TRUE;

    if ((!IsWindowVisible(hWnd)) || (GetWindow(hWnd, GW_OWNER) != NULL))
        return TRUE;

    QMap<QString, QUrl> *windows = reinterpret_cast<QMap<QString, QUrl>*>(lparam);

    int length = GetWindowTextLengthW(hWnd);
    wchar_t* buffer = new wchar_t[length + 1];
    int length2 = GetWindowTextW(hWnd, buffer, length + 1);
    std::wstring windowTitle(buffer);

    // List visible windows with a non-empty title
    if (length != 0 && length2 !=0) {
        //    if (IsWindowVisible(hWnd) && length != 0 && length2!=0) {
        std::wstring path = getWindowPath(hWnd);
        std::wcout << hWnd << ":  " << windowTitle << " Path: " << path << std::endl;

        windows->insert(QString::fromStdWString(windowTitle),QString::fromStdWString(path));

    }
    delete []buffer;
    return TRUE;
}
