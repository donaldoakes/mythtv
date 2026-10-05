// Std
#include <mutex>

// MythTV
#include "libmythbase/mythlogging.h"
#include "mythxdisplay.h"

namespace
{
/*! \brief Custom Xlib error handler.
 *
 * Without this, Xlib's default handler calls exit() for any unhandled X
 * protocol error (e.g. BadRRCrtc when a CRTC disappears because an AV
 * receiver switched its HDMI input away and back). That exit() tears down
 * process statics while other threads (e.g. the logging thread) are still
 * running, causing a use-after-free crash. Log and continue instead.
*/
int MythXErrorHandler(Display *Disp, XErrorEvent *Event)
{
    char buf[256];
    XGetErrorText(Disp, Event->error_code, buf, sizeof(buf));
    LOG(VB_GENERAL, LOG_ERR, QString("MythXDisplay: X Error: %1 (request %2.%3)")
        .arg(buf).arg(Event->request_code).arg(Event->minor_code));
    return 0;
}
}

class MythXLocker
{
  public:
    explicit MythXLocker(MythXDisplay* Disp)
      : m_disp(Disp)
    {
        if (m_disp)
            m_disp->Lock();
    }

    ~MythXLocker()
    {
        if (m_disp)
            m_disp->Unlock();
    }

  private:
    MythXDisplay* m_disp { nullptr };
};

MythXDisplay* MythXDisplay::OpenMythXDisplay(bool Warn /*= true*/)
{
    auto * display = new MythXDisplay();
    if (display && display->Open())
        return display;

    if (Warn)
        LOG(VB_GENERAL, LOG_CRIT, "MythXOpenDisplay() failed");
    delete display;
    return nullptr;
}

void MythXDisplay::SetQtX11Display(const QString& DisplayStr)
{
    s_QtX11Display = DisplayStr;
}

/// \brief Determine if we are running a remote X11 session
bool MythXDisplay::DisplayIsRemote()
{
    bool result = false;
    if (auto * display = MythXDisplay::OpenMythXDisplay(false); display != nullptr)
    {
        QString displayname(DisplayString(display->GetDisplay()));

        // DISPLAY=:x or DISPLAY=unix:x are local
        // DISPLAY=hostname:x is remote
        // DISPLAY=/xxx/xxx/.../org.macosforge.xquartz:x is local OS X
        // x can be numbers n or n.n
        // Anything else including DISPLAY not set is assumed local,
        // in that case we are probably not running under X11
        if (!displayname.isEmpty() && !displayname.startsWith(":") &&
            !displayname.startsWith("unix:") && !displayname.startsWith("/") &&
             displayname.contains(':'))
        {
            result = true;
        }
        delete display;
    }
    return result;
}

MythXDisplay::~MythXDisplay()
{
    MythXLocker locker(this);
    if (m_disp)
        XCloseDisplay(m_disp);
}

/*! \brief Open the display
 *
 * \note If the '-display' command line argument is not set both this function
 * and Qt's xcb platform plugin will pass a null string to XOpenDisplay - which
 * will in turn use the DISPLAY environment variable to determince which X11
 * connection to open. If the '-display' command line argument is used, we set
 * s_QtX11Display and the argument is also passed through to the xcb platform
 * plugin by Qt (via the QApplication constructor).
 * So in all cases, the following code should open the same display that is in use by Qt
 * (and avoids linking to QX11Extras or including private Qt platform headers).
 *
*/
bool MythXDisplay::Open(void)
{
    static std::once_flag s_handlerInstalled;
    std::call_once(s_handlerInstalled, []() { XSetErrorHandler(MythXErrorHandler); });

    MythXLocker locker(this);

    const char *dispCStr = nullptr;
    if (!s_QtX11Display.isEmpty())
        dispCStr = s_QtX11Display.toLatin1().constData();

    m_disp = XOpenDisplay(dispCStr);
    if (!m_disp)
        return false;

    m_screenNum = DefaultScreen(m_disp);
    m_root      = DefaultRootWindow(m_disp);

    return true;
}

void MythXDisplay::Sync(bool Flush)
{
    Lock();
    XSync(m_disp, Flush);
    Unlock();
}
