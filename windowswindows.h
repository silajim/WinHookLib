#ifndef WINDOWSWINDOWS_H
#define WINDOWSWINDOWS_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QMap>
#include <QRect>

#include <Windows.h>
#include <Psapi.h>
#include <memory>

class WindowsWindows;

static WindowsWindows *m_windows;

class WindowsWindows : public QObject
{
    Q_OBJECT
public:
    explicit WindowsWindows(QObject *parent = nullptr);
    ~WindowsWindows();

    QMap<QString,QUrl> getWindows();

signals:
    void ForeGroundWindowChanged(QString path, QRect size);

private:
    HWINEVENTHOOK g_hook=nullptr;

    static std::wstring getWindowPath(HWND hWnd);

    static void CALLBACK  Wineventproc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime);

    static BOOL CALLBACK enumWindowCallback(HWND hWnd, LPARAM lparam);

};

#endif // WINDOWSWINDOWS_H
