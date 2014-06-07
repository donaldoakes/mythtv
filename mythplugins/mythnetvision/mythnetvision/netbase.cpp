#include <QDir>

#include <mythdate.h>
#include <mythdirs.h>
#include <mythdate.h>
#include <mythdialogbox.h>
#include <mythcontext.h>
#include <remotefile.h>
#include <mythcoreutil.h>
#include <mythprogressdialog.h>
#include <metadata/metadataimagedownload.h>
#include <metadata/videoutils.h>

#include "netbase.h"

NetBase::NetBase(MythScreenStack *parent, const char *name)
    : MythScreenType(parent, name),
    m_thumbImage(NULL),
    m_downloadable(NULL),
    m_busyPopup(NULL),
    m_popupStack(GetMythMainWindow()->GetStack("popup stack")),
    m_progressDialog(NULL),
    m_imageDownload(new MetadataImageDownload(this))
{
    gCoreContext->addListener(this);
}

NetBase::~NetBase()
{
    CleanCacheDir();

    qDeleteAll(m_grabberList);
    m_grabberList.clear();

    cleanThumbnailCacheDir();

    delete m_imageDownload;
    m_imageDownload = NULL;

    gCoreContext->removeListener(this);
}

void NetBase::Init()
{
    loadData();
}

void NetBase::DownloadVideo(const QString &url, const QString &dest)
{
    InitProgressDialog();
    m_downloadFile = RemoteDownloadFile(url, "Default", dest);
}

void NetBase::InitProgressDialog()
{
    QString message = tr("Downloading Video...");
    m_progressDialog = new MythUIProgressDialog(message,
        m_popupStack, "videodownloadprogressdialog");

    if (m_progressDialog->Create())
    {
        m_popupStack->AddScreen(m_progressDialog, false);
    }
    else
    {
        delete m_progressDialog;
        m_progressDialog = NULL;
    }
}

void NetBase::CreateBusyDialog(const QString &title)
{
    if (m_busyPopup)
        return;

    m_busyPopup = new MythUIBusyDialog(title, m_popupStack,
        "netvisionbusydialog");

    if (m_busyPopup->Create())
        m_popupStack->AddScreen(m_busyPopup);
    else
    {
        delete m_busyPopup;
        m_busyPopup = NULL;
    }
}

void NetBase::CleanCacheDir()
{
    QString cache = QString("%1/cache/netvision-thumbcache")
                       .arg(GetConfDir());
    QDir cacheDir(cache);
    QStringList thumbs = cacheDir.entryList(QDir::Files);

    for (QStringList::const_iterator i = thumbs.end() - 1;
            i != thumbs.begin() - 1; --i)
    {
        QString filename = QString("%1/%2").arg(cache).arg(*i);
        LOG(VB_GENERAL, LOG_DEBUG, QString("Deleting file %1").arg(filename));
        QFileInfo fi(filename);
        QDateTime lastmod = fi.lastModified();
        if (lastmod.addDays(7) < MythDate::current())
            QFile::remove(filename);
    }
}

void NetBase::streamWebVideo()
{
    ResultItem *item = GetStreamItem();

    if (!item)
        return;

    if (!item->GetDownloadable())
    {
        showWebVideo();
        return;
    }

    GetMythMainWindow()->HandleMedia(
        "Internal", item->GetMediaURL(),
        item->GetDescription(), item->GetTitle(), item->GetSubtitle(),
        QString(), item->GetSeason(), item->GetEpisode(), QString(),
        item->GetTime().toInt(), item->GetDate().toString("yyyy"));
}

void NetBase::showWebVideo()
{
    ResultItem *item = GetStreamItem();

    if (!item)
        return;

    if (!item->GetPlayer().isEmpty())
    {
        QString cmd = item->GetPlayer();
        QStringList args = item->GetPlayerArguments();
        if (!args.size())
        {
            args += item->GetMediaURL();
            if (!args.size())
                args += item->GetURL();
        }
        else
        {
            args.replaceInStrings("%DIR%", QString(GetConfDir() + "/MythNetvision"));
            args.replaceInStrings("%MEDIAURL%", item->GetMediaURL());
            args.replaceInStrings("%URL%", item->GetURL());
            args.replaceInStrings("%TITLE%", item->GetTitle());
        }
        QString playerCommand = cmd + " " + args.join(" ");
        RunCmdWithoutScreensaver(playerCommand);
    }
    else
    {
        QString url = item->GetURL();

        LOG(VB_GENERAL, LOG_DEBUG, QString("Web URL = %1").arg(url));

        if (url.isEmpty())
            return;

        QString browser = gCoreContext->GetSetting("WebBrowserCommand", "");
        QString zoom = gCoreContext->GetSetting("WebBrowserZoomLevel", "1.0");

        if (browser.isEmpty())
        {
            ShowOkPopup(tr("No browser command set! MythNetTree needs MythBrowser "
                           "installed to display the video."));
            return;
        }

        if (browser.toLower() == "internal")
        {
            GetMythMainWindow()->HandleMedia("WebBrowser", url);
        }
        else
        {
            url.replace("mythflash://", "http://");
            QString cmd = browser;
            cmd.replace("%ZOOM%", zoom);
            cmd.replace("%URL%", url);
            cmd.replace('\'', "%27");
            cmd.replace("&","\\&");
            cmd.replace(";","\\;");

            RunCmdWithoutScreensaver(cmd);
        }
    }
}

void NetBase::RunCmdWithoutScreensaver(const QString &cmd)
{
    GetMythMainWindow()->PauseIdleTimer(true);
    GetMythUI()->DisableScreensaver();
    GetMythMainWindow()->AllowInput(false);
    myth_system(cmd, kMSDontDisableDrawing);
    GetMythMainWindow()->AllowInput(true);
    GetMythMainWindow()->PauseIdleTimer(false);
    GetMythUI()->RestoreScreensaver();
}

void NetBase::slotDeleteVideo()
{
    QString message = tr("Are you sure you want to delete this file?");

    MythConfirmationDialog *confirmdialog =
        new MythConfirmationDialog(m_popupStack, message);

    if (confirmdialog->Create())
    {
        m_popupStack->AddScreen(confirmdialog);
        connect(confirmdialog, SIGNAL(haveResult(bool)),
                SLOT(doDeleteVideo(bool)));
    }
    else
        delete confirmdialog;
}

void NetBase::doDeleteVideo(bool remove)
{
    if (!remove)
        return;

    ResultItem *item = GetStreamItem();

    if (!item)
        return;

    QString filename = GetDownloadFilename(item->GetTitle(),
                                           item->GetMediaURL());

    if (filename.startsWith("myth://"))
        RemoteFile::DeleteFile(filename);
    else
    {
        QFile file(filename);
        file.remove();
    }
}

void NetBase::customEvent(QEvent *event)
{
    if ((MythEvent::Type)(event->type()) == MythEvent::MythEventMessage)
    {
        MythEvent *me = (MythEvent *)event;
        QStringList tokens = me->Message().split(" ", QString::SkipEmptyParts);

        if (tokens.isEmpty())
            return;

        if (tokens[0] == "DOWNLOAD_FILE")
        {
            QStringList args = me->ExtraDataList();
            if ((tokens.size() != 2) ||
                (args[1] != m_downloadFile))
                return;

            if (tokens[1] == "UPDATE")
            {
                QString message = tr("Downloading Video...\n"
                                     "(%1 of %2 MB)")
                    .arg(QString::number(args[2].toInt() / 1024.0 / 1024.0, 'f', 2))
                    .arg(QString::number(args[3].toInt() / 1024.0 / 1024.0, 'f', 2));
                m_progressDialog->SetMessage(message);
                m_progressDialog->SetTotal(args[3].toInt());
                m_progressDialog->SetProgress(args[2].toInt());
            }
            else if (tokens[1] == "FINISHED")
            {
                int fileSize  = args[2].toInt();
                int errorCode = args[4].toInt();

                if (m_progressDialog)
                    m_progressDialog->Close();

                if ((m_downloadFile.startsWith("myth://")))
                {
                    if ((errorCode == 0) &&
                        (fileSize > 0))
                    {
                        doPlayVideo(m_downloadFile);
                    }
                    else
                    {
                        ShowOkPopup(tr("Error downloading video to backend."));
                    }
                }
            }
        }
    }
}

void NetBase::doDownloadAndPlay()
{
    ResultItem *item = GetStreamItem();
    if (!item)
        return;

    QString baseFilename = GetDownloadFilename(item->GetTitle(),
                                          item->GetMediaURL());

    QString finalFilename = generate_file_url("Default",
                              gCoreContext->GetMasterHostName(),
                              baseFilename);

    LOG(VB_GENERAL, LOG_INFO, QString("Downloading %1 to %2")
        .arg(item->GetMediaURL()) .arg(finalFilename));

    // Does the file already exist?
    bool exists = RemoteFile::Exists(finalFilename);

    if (exists)
    {
        doPlayVideo(finalFilename);
        return;
    }
    else
        DownloadVideo(item->GetMediaURL(), baseFilename);
}

void NetBase::doPlayVideo(const QString &filename)
{
    ResultItem *item = GetStreamItem();
    if (!item)
        return;

    GetMythMainWindow()->HandleMedia("Internal", filename);
}
