#include "windowswindows.h"
#include <iostream>


extern WindowsWindows *m_windows;

WindowsWindows::WindowsWindows(QObject *parent) : QObject(parent)
{
    m_windows = this;

    CoInitializeEx(NULL,COINIT_MULTITHREADED);
    g_hook = SetWinEventHook(
                EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
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
    WCHAR *Windowname;
    if(hwnd!=NULL){
        int length = GetWindowTextLengthW(hwnd);
        RECT r;
        GetWindowRect(hwnd,&r);
        QRect rect(r.left,r.top,r.right - r.left , r.bottom - r.top);
        Windowname = new wchar_t[length + 1];
        GetWindowTextW(hwnd,&Windowname[0],length+1);
        std::wcout << L"Window Name " << Windowname << L" Path " << getWindowPath(hwnd) << std::endl;
        m_windows->ForeGroundWindowChanged(QString::fromStdWString(getWindowPath(hwnd)),rect);
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
