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

#include <emmintrin.h>
#include <smmintrin.h>

class WindowsWindows;

static WindowsWindows *m_windows;

class WindowsWindows : public QObject
{
    Q_OBJECT
public:
    explicit WindowsWindows(QObject *parent = nullptr);
    ~WindowsWindows();

    QMap<QString,QUrl> getWindows();

    static LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam,LPARAM lParam);
    HHOOK windowHook = nullptr;
    HWND currentForeground=nullptr;
    RECT currentRecct;


signals:
    void ForeGroundWindowChanged(QString path, QString name ,QRect size, bool fullscreen);

private:
    HWINEVENTHOOK g_hook=nullptr;

    static bool equalRect(const RECT &rect1,const RECT &rect2);
     static bool equalRect2(const LONG *a, const LONG *b);

    static inline bool compare_longs(const long& a, const long& b) {
        __m128i xmm_a = _mm_loadu_si128((__m128i*)&a);
        __m128i xmm_b = _mm_loadu_si128((__m128i*)&b);
        __m128i xmm_result = _mm_cmpeq_epi64(xmm_a, xmm_b);
        int result = _mm_movemask_epi8(xmm_result);
        return result == 0xFFFF;
    }

    static std::wstring getWindowPath(HWND hWnd);

    static void CALLBACK  Wineventproc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime);

    static BOOL CALLBACK enumWindowCallback(HWND hWnd, LPARAM lparam);




};

#endif // WINDOWSWINDOWS_H
