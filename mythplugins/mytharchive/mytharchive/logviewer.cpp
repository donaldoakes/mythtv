#include <unistd.h>
#include <iostream>

using namespace std;

// qt
#include <qlayout.h>
#include <qhbox.h>
#include <qfile.h>

// mythtv
#include <mythtv/mythcontext.h>
#include <mythtv/dialogbox.h>
#include <mythtv/mythdialogs.h>
#include <mythtv/mythdbcon.h>

// mytharchive
#include <logviewer.h>

const int DEFAULT_UPDATE_TIME = 5;

LogViewer::LogViewer(MythMainWindow *parent, const char *name)
              : MythDialog(parent, name)
{
    m_updateTime = gContext->GetNumSetting("LogViewerUpdateTime", DEFAULT_UPDATE_TIME);

    QVBoxLayout *vbox = new QVBoxLayout(this, (int)(15 * wmult));
    QHBoxLayout *hbox = new QHBoxLayout(vbox, (int)(0 * wmult));

    // Window title
    QString message = tr("Log Viewer");
    QLabel *label = new QLabel(message, this);
    QFont font = label->font();
    font.setPointSize(int (font.pointSize() * 1.2));
    font.setBold(true);
    label->setFont(font);
    label->setPaletteForegroundColor(QColor("yellow"));
    label->setBackgroundOrigin(WindowOrigin);
    label->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    hbox->addWidget(label);

    hbox = new QHBoxLayout(vbox, (int)(10 * wmult));
    m_autoupdateCheck = new MythCheckBox( this );
    m_autoupdateCheck->setBackgroundOrigin(WindowOrigin);
    m_autoupdateCheck->setChecked(true);
    m_autoupdateCheck->setText("Auto Update Frequency");
    hbox->addWidget(m_autoupdateCheck);

    m_updateTimeSpin = new MythSpinBox( this );
    m_updateTimeSpin->setMinValue(1);
    m_updateTimeSpin->setValue(m_updateTime);
    hbox->addWidget(m_updateTimeSpin);

    message = tr("Seconds");
    label = new QLabel(message, this);
    label->setBackgroundOrigin(WindowOrigin);
    label->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    hbox->addWidget(label);

    // listbox
    hbox = new QHBoxLayout(vbox, (int)(10 * wmult));

    m_listbox = new MythListBox( this );
    m_listbox->setBackgroundOrigin(WindowOrigin);
    m_listbox->setEnabled(true);
    font = m_listbox->font();
    font.setPointSize(gContext->GetNumSetting("LogViewerFontSize", 13));
    font.setBold(false);
    m_listbox->setFont(font);
    hbox->addWidget(m_listbox);


    hbox = new QHBoxLayout(vbox, (int)(10 * wmult));

    //  cancel Button
    hbox = new QHBoxLayout(vbox, (int)(10 * wmult));

    m_cancelButton = new MythPushButton( this, "cancel" );
    m_cancelButton->setBackgroundOrigin(WindowOrigin);
    m_cancelButton->setText( tr( "Cancel" ) );
    m_cancelButton->setEnabled(true);

    hbox->addWidget(m_cancelButton);

    //  update Button
    m_updateButton = new MythPushButton( this, "update" );
    m_updateButton->setBackgroundOrigin(WindowOrigin);
    m_updateButton->setText( tr( "Update" ) );
    m_updateButton->setEnabled(true);
    m_updateButton->setFocus();

    hbox->addWidget(m_updateButton);

    // exit button
    m_exitButton = new MythPushButton( this, "exit" );
    m_exitButton->setBackgroundOrigin(WindowOrigin);
    m_exitButton->setText( tr( "Exit" ) );
    m_exitButton->setEnabled(true);

    hbox->addWidget(m_exitButton);

    connect(m_exitButton, SIGNAL(clicked()), this, SLOT(exitClicked()));
    connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(m_updateButton, SIGNAL(clicked()), this, SLOT(updateClicked()));
    connect(m_autoupdateCheck, SIGNAL(toggled(bool)), this, SLOT(toggleAutoUpdate(bool)));
    connect(m_updateTimeSpin, SIGNAL(valueChanged(int)), 
            this, SLOT(updateTimeChanged(int)));

    m_updateTimer = NULL;
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, SIGNAL(timeout()), SLOT(updateTimerTimeout()) );
    m_updateTimer->start(500);

    m_popupMenu = NULL;
}

LogViewer::~LogViewer(void)
{
    gContext->SaveSetting("LogViewerUpdateTime", m_updateTime);
    gContext->SaveSetting("LogViewerFontSize", m_listbox->font().pointSize());

    if (m_updateTimer)
        delete m_updateTimer;
}

void LogViewer::updateTimerTimeout()
{
    updateClicked();
}

void LogViewer::toggleAutoUpdate(bool checked)
{
    if (checked)
    {
        m_updateTimeSpin->setEnabled(true);
        m_updateTimer->start(m_updateTime * 1000);
    }
    else
    {
        m_updateTimeSpin->setEnabled(false);
        m_updateTimer->stop();
    }
}

void LogViewer::updateTimeChanged(int value)
{
    m_updateTime = value;
    m_updateTimer->stop();
    m_updateTimer->changeInterval(value * 1000);
}

void LogViewer::exitClicked(void)
{
    done(-1);
}

void LogViewer::cancelClicked(void)
{
    QString tempDir = gContext->GetSetting("MythArchiveTempDir", "");

    system("echo Cancel > " + tempDir + "/logs/mythburncancel.lck" );

    MythPopupBox::showOkPopup(gContext->GetMainWindow(), 
        QObject::tr("Myth Burn"),
        QObject::tr("Background creation has been asked to stop.\n" 
                                "This may take a few minutes."));
}

void LogViewer::updateClicked(void)
{
    m_updateTimer->stop();

    QStringList list;
    loadFile(m_currentLog, list, m_listbox->count());

    if (list.count() > 0)
    {
        bool bUpdateCurrent = m_listbox->currentItem() == (int) m_listbox->count() - 1;
        m_listbox->insertStringList(list);
        if (bUpdateCurrent)
            m_listbox->setCurrentItem(m_listbox->count() - 1);
    }

    if (m_autoupdateCheck->isChecked())
        m_updateTimer->start(m_updateTime * 1000);
}

bool LogViewer::loadFile(QString filename, QStringList &list, int startline)
{
    list.clear();

    QFile file(filename);

    if (!file.exists())
        return false;

    if (file.open( IO_ReadOnly ))
    {
        QString s;
        QTextStream stream(&file);

         // ignore the first startline lines
        while ( !stream.atEnd() && startline > 0)
        {
            stream.readLine();
            startline--;
        }

         // read rest of file
        while ( !stream.atEnd() )
        {
            s = stream.readLine();
            list.append(s);
        }
        file.close();
    }
    else
        return false;

    return true;
}

void LogViewer::keyPressEvent(QKeyEvent *e)
{
    bool handled = false;
    QStringList actions;
    if (gContext->GetMainWindow()->TranslateKeyPress("qt", e, actions))
    {
        for (unsigned int i = 0; i < actions.size() && !handled; i++)
        {
            QString action = actions[i];
            if (action == "1")
            {
                handled = true;
                decreaseFontSize();
            }
            else if (action == "2")
            {
                handled = true;
                increaseFontSize();
            }
            else if (action == "3")
            {
                handled = true;
                showProgressLog();
            }
            else if (action == "4")
            {
                handled = true;
                showFullLog();
            }
            else if (action == "MENU")
            {
                handled = true;
                showMenu();
            }
        }
    }

    if (!handled)
        MythDialog::keyPressEvent(e);
}

void LogViewer::increaseFontSize(void)
{
    closePopupMenu();

    QFont font = m_listbox->font();
    font.setPointSize(font.pointSize() + 1);
    m_listbox->setFont(font);
}

void LogViewer::decreaseFontSize(void)
{
    closePopupMenu();

    QFont font = m_listbox->font();
    font.setPointSize(font.pointSize() - 1);
    m_listbox->setFont(font);
}

void LogViewer::setFilenames(const QString &progressLog, const QString &fullLog)
{
    m_progressLog = progressLog;
    m_fullLog = fullLog;
    m_currentLog = progressLog;
}

void LogViewer::showProgressLog(void)
{
    closePopupMenu();

    m_listbox->clear();
    m_currentLog = m_progressLog;
    updateClicked();
}

void LogViewer::showFullLog(void)
{
    closePopupMenu();

    m_listbox->clear();
    m_currentLog = m_fullLog;
    updateClicked();
}

void LogViewer::showMenu()
{
    if (m_popupMenu)
        return;

    m_popupMenu = new MythPopupBox(gContext->GetMainWindow(),
                                      "logviewer menu");

    QButton *button = m_popupMenu->addButton(tr("Increase Font Size"), this,
            SLOT(increaseFontSize()));

    m_popupMenu->addButton(tr("Decrease Font Size"), this,
                              SLOT(decreaseFontSize()));

    m_popupMenu->addButton(tr("Show Progress Log"), this,
                              SLOT(showProgressLog()));
    m_popupMenu->addButton(tr("Show Full Log"), this,
                           SLOT(showFullLog()));
    m_popupMenu->addButton(tr("Cancel"), this,
                           SLOT(closePopupMenu()));
    m_popupMenu->ShowPopup(this, SLOT(closePopupMenu()));

    button->setFocus();
}

void LogViewer::closePopupMenu()
{
    if (!m_popupMenu)
        return;

    m_popupMenu->hide();
    delete m_popupMenu;
    m_popupMenu = NULL;
}
