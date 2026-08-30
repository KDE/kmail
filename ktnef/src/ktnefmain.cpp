/*
  This file is part of KTnef.

  SPDX-FileCopyrightText: 2002 Michael Goffioul <kdeprint@swing.be>
  SPDX-FileCopyrightText: 2012 Allen Winter <winter@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
*/

#include "ktnefmain.h"

#include "attachpropertydialog.h"
#include "ktnefview.h"
#include "messagepropertydialog.h"

#include <KTNEF/KTNEFMessage>
#include <KTNEF/KTNEFParser>
#include <KTNEF/KTNEFProperty>

#include "ktnef_debug.h"
#include <KActionCollection>
#include <KEditToolBar>
#include <KFileItemActions>
#include <KIO/ApplicationLauncherJob>
#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KMessageBox>
#include <KShortcutsDialog>
#include <KStandardAction>
#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QPointer>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QUrl>

#include <KConfigGroup>
#include <KRecentFilesMenu>

#include <KSharedConfig>
#include <QActionGroup>
#include <QContextMenuEvent>
#include <QDir>
#include <QDrag>
#include <QFileDialog>
#include <QMimeData>
#include <QMimeDatabase>
#include <QStatusBar>

using namespace Qt::Literals::StringLiterals;
KTNEFMain::KTNEFMain(QWidget *parent)
    : KXmlGuiWindow(parent)
{
    setupActions();
    setupStatusbar();

    setupTNEF();

    KConfigGroup config(KSharedConfig::openConfig(), u"Settings"_s);
    mDefaultDir = config.readPathEntry("defaultdir", u"/tmp/"_s);

    mLastDir = mDefaultDir;

    // create personal temp extract dir
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/ktnef/"_L1);

    resize(430, 350);

    setStandardToolBarMenuEnabled(true);

    createStandardStatusBarAction();

    setupGUI(Keys | Save | Create, u"ktnefui.rc"_s);

    setAutoSaveSettings();
}

KTNEFMain::~KTNEFMain()
{
    delete mParser;
    cleanup();
}

void KTNEFMain::setupActions()
{
    KStandardActions::quit(this, &KTNEFMain::close, actionCollection());

    QAction *action = KStandardActions::keyBindings(this, &KTNEFMain::slotConfigureKeys, actionCollection());
    action->setWhatsThis(i18nc("@info:whatsthis",
                               "You will be presented with a dialog where you can configure "
                               "the application-wide shortcuts."));

    KStandardActions::configureToolbars(this, &KTNEFMain::slotEditToolbars, actionCollection());

    // File menu
    KStandardActions::open(this, &KTNEFMain::openFile, actionCollection());

    mOpenRecentFileMenu = new KRecentFilesMenu(this);
    actionCollection()->addAction(u"ktnef_file_open_recent"_s, mOpenRecentFileMenu->menuAction());
    connect(mOpenRecentFileMenu, &KRecentFilesMenu::urlTriggered, this, &KTNEFMain::openRecentFile);

    // Action menu
    QAction *openAction = actionCollection()->addAction(u"view_file"_s);
    openAction->setText(i18nc("@action:inmenu", "View"));
    openAction->setIcon(QIcon::fromTheme(u"document-open"_s));
    connect(openAction, &QAction::triggered, this, &KTNEFMain::viewFile);

    QAction *openAsAction = actionCollection()->addAction(u"view_file_as"_s);
    openAsAction->setText(i18nc("@action:inmenu", "View With…"));
    connect(openAsAction, &QAction::triggered, this, &KTNEFMain::viewFileAs);

    QAction *extractAction = actionCollection()->addAction(u"extract_file"_s);
    extractAction->setText(i18nc("@action:inmenu", "Extract"));
    connect(extractAction, &QAction::triggered, this, &KTNEFMain::extractFile);

    QAction *extractToAction = actionCollection()->addAction(u"extract_file_to"_s);
    extractToAction->setText(i18nc("@action:inmenu", "Extract To…"));
    extractToAction->setIcon(QIcon::fromTheme(u"archive-extract"_s));
    connect(extractToAction, &QAction::triggered, this, &KTNEFMain::extractFileTo);

    QAction *extractAllToAction = actionCollection()->addAction(u"extract_all_files"_s);
    extractAllToAction->setText(i18nc("@action:inmenu", "Extract All To…"));
    extractAllToAction->setIcon(QIcon::fromTheme(u"archive-extract"_s));
    connect(extractAllToAction, &QAction::triggered, this, &KTNEFMain::extractAllFiles);

    QAction *filePropsAction = actionCollection()->addAction(u"properties_file"_s);
    filePropsAction->setText(i18nc("@action:inmenu", "Properties"));
    filePropsAction->setIcon(QIcon::fromTheme(u"document-properties"_s));
    connect(filePropsAction, &QAction::triggered, this, &KTNEFMain::propertiesFile);

    QAction *messPropsAction = actionCollection()->addAction(u"msg_properties"_s);
    messPropsAction->setText(i18nc("@action:inmenu", "Message Properties"));
    connect(messPropsAction, &QAction::triggered, this, &KTNEFMain::slotShowMessageProperties);

    QAction *messShowAction = actionCollection()->addAction(u"msg_text"_s);
    messShowAction->setText(i18nc("@action:inmenu", "Show Message Text"));
    messShowAction->setIcon(QIcon::fromTheme(u"document-preview-archive"_s));
    connect(messShowAction, &QAction::triggered, this, &KTNEFMain::slotShowMessageText);

    QAction *messSaveAction = actionCollection()->addAction(u"msg_save"_s);
    messSaveAction->setText(i18nc("@action:inmenu", "Save Message Text As…"));
    messSaveAction->setIcon(QIcon::fromTheme(u"document-save"_s));
    connect(messSaveAction, &QAction::triggered, this, &KTNEFMain::slotSaveMessageText);

    actionCollection()->action(u"view_file"_s)->setEnabled(false);
    actionCollection()->action(u"view_file_as"_s)->setEnabled(false);
    actionCollection()->action(u"extract_file"_s)->setEnabled(false);
    actionCollection()->action(u"extract_file_to"_s)->setEnabled(false);
    actionCollection()->action(u"extract_all_files"_s)->setEnabled(false);
    actionCollection()->action(u"properties_file"_s)->setEnabled(false);

    // Options menu
    QAction *defFolderAction = actionCollection()->addAction(u"options_default_dir"_s);
    defFolderAction->setText(i18nc("@action:inmenu", "Default Folder…"));
    defFolderAction->setIcon(QIcon::fromTheme(u"folder-open"_s));
    connect(defFolderAction, &QAction::triggered, this, &KTNEFMain::optionDefaultDir);
}

void KTNEFMain::slotConfigureKeys()
{
    KShortcutsDialog::showDialog(actionCollection(), KShortcutsEditor::LetterShortcutsAllowed, this);
}

void KTNEFMain::setupStatusbar()
{
    statusBar()->showMessage(i18nc("@info:status", "No file loaded"));
}

void KTNEFMain::setupTNEF()
{
    mView = new KTNEFView(this);
    mView->setAllColumnsShowFocus(true);
    mParser = new KTNEFParser;

    setCentralWidget(mView);

    connect(mView, &QTreeWidget::itemSelectionChanged, this, &KTNEFMain::viewSelectionChanged);

    connect(mView, &QTreeWidget::itemDoubleClicked, this, &KTNEFMain::viewDoubleClicked);
}

void KTNEFMain::loadFile(const QString &filename)
{
    mFilename = filename;
    setCaption(mFilename);
    if (!mParser->openFile(filename)) {
        mView->setAttachments(QList<KTNEFAttach *>());
        enableExtractAll(false);
        KMessageBox::error(this, i18nc("@info", "Unable to open file \"%1\".", filename));
    } else {
        addRecentFile(QUrl::fromLocalFile(filename));
        const QList<KTNEFAttach *> list = mParser->message()->attachmentList();
        QString msg;
        msg = i18ncp("@info:status", "%1 attachment found", "%1 attachments found", list.count());
        statusBar()->showMessage(msg);
        mView->setAttachments(list);
        enableExtractAll(!list.isEmpty());
        enableSingleAction(false);
    }
}

void KTNEFMain::openFile()
{
    if (const QString filename = QFileDialog::getOpenFileName(this, i18nc("@title:window", "Open TNEF File")); !filename.isEmpty()) {
        loadFile(filename);
    }
}

void KTNEFMain::openRecentFile(const QUrl &url)
{
    loadFile(url.path());
}

void KTNEFMain::addRecentFile(const QUrl &url)
{
    mOpenRecentFileMenu->addUrl(url);
}

void KTNEFMain::viewFile()
{
    if (!mView->getSelection().isEmpty()) {
        KTNEFAttach *attach = mView->getSelection().at(0);
        const QUrl url = QUrl::fromLocalFile(extractTemp(attach));
        QString mimename(attach->mimeTag());

        if (mimename.isEmpty() || mimename == "application/octet-stream"_L1) {
            qCDebug(KTNEFAPPS_LOG) << "No mime type found in attachment object, trying to guess...";
            QMimeDatabase db;
            mimename = db.mimeTypeForFile(url.path(), QMimeDatabase::MatchExtension).name();
            qCDebug(KTNEFAPPS_LOG) << "Detected mime type: " << mimename;
        } else {
            qCDebug(KTNEFAPPS_LOG) << "Mime type from attachment object: " << mimename;
        }
        auto job = new KIO::OpenUrlJob(url, mimename);
        job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
        job->setDeleteTemporaryFile(true);
        job->start();
    } else {
        KMessageBox::information(this, i18nc("@info", "There is no file selected. Please select a file an try again."));
    }
}

QString KTNEFMain::extractTemp(KTNEFAttach *att)
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/ktnef/"_L1;
    mParser->extractFileTo(att->name(), dir);
    QString filename = att->fileName();
    // falling back to internal TNEF attachment name if no filename is given for the attached file
    // this follows the logic of KTNEFParser::extractFileTo(...)
    if (filename.isEmpty()) {
        filename = att->name();
    }
    dir.append(filename);
    return dir;
}

void KTNEFMain::viewFileAs()
{
    if (!mView->getSelection().isEmpty()) {
        if (const QList<QUrl> list{QUrl::fromLocalFile(extractTemp(mView->getSelection().at(0)))}; !list.isEmpty()) {
            // Creating ApplicationLauncherJob without any args will invoke the open-with dialog
            auto job = new KIO::ApplicationLauncherJob();
            job->setUrls(list);
            job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
            job->start();
        }
    } else {
        KMessageBox::information(this, i18nc("@info", "There is no file selected. Please select a file an try again."));
    }
}

void KTNEFMain::extractFile()
{
    extractTo(mDefaultDir);
}

void KTNEFMain::extractFileTo()
{
    if (const QString dir = QFileDialog::getExistingDirectory(this, QString(), mLastDir); !dir.isEmpty()) {
        extractTo(dir);
        mLastDir = dir;
    }
}

void KTNEFMain::extractAllFiles()
{
    if (QString dir = QFileDialog::getExistingDirectory(this, QString(), mLastDir); !dir.isEmpty()) {
        mLastDir = dir;
        dir.append(u'/');
        const QList<KTNEFAttach *> list = mParser->message()->attachmentList();
        QList<KTNEFAttach *>::ConstIterator it;
        QList<KTNEFAttach *>::ConstIterator end(list.constEnd());
        for (it = list.constBegin(); it != end; ++it) {
            if (!mParser->extractFileTo((*it)->name(), dir)) {
                KMessageBox::error(this, i18nc("@info", "Unable to extract file \"%1\".", (*it)->name()));
                return;
            }
        }
    }
}

void KTNEFMain::propertiesFile()
{
    KTNEFAttach *attach = mView->getSelection().at(0);
    AttachPropertyDialog dlg(this);
    dlg.setAttachment(attach);
    dlg.exec();
}

void KTNEFMain::optionDefaultDir()
{
    if (const QString dirname = QFileDialog::getExistingDirectory(this, QString(), mDefaultDir); !dirname.isEmpty()) {
        mDefaultDir = dirname;

        KConfigGroup config(KSharedConfig::openConfig(), u"Settings"_s);
        config.writePathEntry("defaultdir", mDefaultDir);
    }
}

void KTNEFMain::viewSelectionChanged()
{
    const QList<KTNEFAttach *> list = mView->getSelection();
    const int nbItem = list.count();
    const bool on1 = (nbItem == 1);
    const bool on2 = (nbItem > 0);

    actionCollection()->action(u"view_file"_s)->setEnabled(on1);
    actionCollection()->action(u"view_file_as"_s)->setEnabled(on1);
    actionCollection()->action(u"properties_file"_s)->setEnabled(on1);

    actionCollection()->action(u"extract_file"_s)->setEnabled(on2);
    actionCollection()->action(u"extract_file_to"_s)->setEnabled(on2);
}

void KTNEFMain::enableExtractAll(bool on)
{
    if (!on) {
        enableSingleAction(false);
    }

    actionCollection()->action(u"extract_all_files"_s)->setEnabled(on);
}

void KTNEFMain::enableSingleAction(bool on)
{
    actionCollection()->action(u"extract_file"_s)->setEnabled(on);
    actionCollection()->action(u"extract_file_to"_s)->setEnabled(on);
    actionCollection()->action(u"view_file"_s)->setEnabled(on);
    actionCollection()->action(u"view_file_as"_s)->setEnabled(on);
    actionCollection()->action(u"properties_file"_s)->setEnabled(on);
}

void KTNEFMain::cleanup()
{
    QDir d(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/ktnef/"_L1);
    d.removeRecursively();
}

void KTNEFMain::extractTo(const QString &dirname)
{
    QString dir = dirname;
    if (!dir.endsWith(u'/')) {
        dir.append(u'/');
    }
    const QList<KTNEFAttach *> list = mView->getSelection();
    QList<KTNEFAttach *>::ConstIterator it;
    QList<KTNEFAttach *>::ConstIterator end(list.constEnd());
    for (it = list.constBegin(); it != end; ++it) {
        if (!mParser->extractFileTo((*it)->name(), dir)) {
            KMessageBox::error(this, i18nc("@info", "Unable to extract file \"%1\".", (*it)->name()));
            return;
        }
    }
}

void KTNEFMain::contextMenuEvent(QContextMenuEvent *event)
{
    const QList<KTNEFAttach *> list = mView->getSelection();
    if (list.isEmpty()) {
        return;
    }

    QAction *prop = nullptr;
    QMenu menu(this);
    if (list.count() == 1) {
        createOpenWithMenu(&menu);
        menu.addSeparator();
    }
    const QAction *extract = menu.addAction(i18nc("@action:inmenu", "Extract"));
    const QAction *extractTo = menu.addAction(QIcon::fromTheme(u"archive-extract"_s), i18nc("@action:inmenu", "Extract To…"));
    if (list.count() == 1) {
        menu.addSeparator();
        prop = menu.addAction(QIcon::fromTheme(u"document-properties"_s), i18nc("@action:inmenu", "Properties"));
    }

    const QAction *a = menu.exec(event->globalPos(), nullptr);
    if (a) {
        if (a == extract) {
            extractFile();
        } else if (a == extractTo) {
            extractFileTo();
        } else if (a == prop) {
            propertiesFile();
        }
    }
}

void KTNEFMain::viewDoubleClicked(QTreeWidgetItem *item)
{
    if (item && item->isSelected()) {
        viewFile();
    }
}

void KTNEFMain::viewDragRequested(const QList<KTnef::KTNEFAttach *> &list)
{
    QList<QUrl> urlList;
    urlList.reserve(list.count());
    for (const auto &att : list) {
        urlList << QUrl::fromLocalFile(extractTemp(att));
    }

    if (!list.isEmpty()) {
        auto mimeData = new QMimeData;
        mimeData->setUrls(urlList);

        auto drag = new QDrag(this);
        drag->setMimeData(mimeData);
    }
}

void KTNEFMain::slotEditToolbars()
{
    KConfigGroup grp = KSharedConfig::openConfig()->group(u"MainWindow"_s);
    saveMainWindowSettings(grp);

    QPointer<KEditToolBar> dlg = new KEditToolBar(factory());
    connect(dlg.data(), &KEditToolBar::newToolBarConfig, this, &KTNEFMain::slotNewToolbarConfig);
    dlg->exec();
    delete dlg;
}

void KTNEFMain::slotNewToolbarConfig()
{
    createGUI(u"ktnefui.rc"_s);
    applyMainWindowSettings(KSharedConfig::openConfig()->group(u"MainWindow"_s));
}

void KTNEFMain::slotShowMessageProperties()
{
    MessagePropertyDialog dlg(this, mParser->message());
    dlg.exec();
}

void KTNEFMain::slotShowMessageText()
{
    if (!mParser->message()) {
        return;
    }

    if (const QString rtf = mParser->message()->rtfString(); !rtf.isEmpty()) {
        auto tmpFile = new QTemporaryFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/ktnef/"_L1 + "ktnef_XXXXXX.rtf"_L1);
        if (!tmpFile->open()) {
            qCWarning(KTNEFAPPS_LOG) << "Impossible to open temporary file";
            delete tmpFile;
            return;
        }
        tmpFile->setAutoRemove(false);
        tmpFile->setPermissions(QFile::ReadUser);
        tmpFile->write(rtf.toLocal8Bit());
        tmpFile->close();
        auto job = new KIO::OpenUrlJob(QUrl::fromLocalFile(tmpFile->fileName()), u"text/rtf"_s);
        job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
        job->setDeleteTemporaryFile(true);
        job->start();
        delete tmpFile;
    } else {
        KMessageBox::error(this, i18nc("@info", "The message does not contain any Rich Text data."));
    }
}

void KTNEFMain::slotSaveMessageText()
{
    if (!mParser->message()) {
        return;
    }

    const QString rtf = mParser->message()->rtfString();
    const QString filename = QFileDialog::getSaveFileName(this, QString(), QString(), QString());
    if (!filename.isEmpty()) {
        if (QFile f(filename); f.open(QIODevice::WriteOnly)) {
            QTextStream t(&f);
            t << rtf;
        } else {
            KMessageBox::error(this, i18nc("@info", "Unable to open file \"%1\" for writing, check file permissions.", filename));
        }
    }
}

void KTNEFMain::openWith(const KService::Ptr &offer)
{
    if (!mView->getSelection().isEmpty()) {
        KTNEFAttach *attach = mView->getSelection().at(0);
        const QUrl url = QUrl::fromLocalFile(extractTemp(attach));
        const QList<QUrl> lst{url};
        auto job = new KIO::ApplicationLauncherJob(offer);
        job->setUrls(lst);
        job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
        job->start();
    }
}

QAction *KTNEFMain::createAppAction(const KService::Ptr &service, bool singleOffer, QActionGroup *actionGroup, QObject *parent)
{
    QString actionName(service->name().replace(u'&', u"&&"_s));
    if (singleOffer) {
        actionName = i18n("Open &with %1", actionName);
    } else {
        actionName = i18nc("@item:inmenu Open With, %1 is application name", "%1", actionName);
    }

    auto act = new QAction(parent);
    act->setIcon(QIcon::fromTheme(service->icon()));
    act->setText(actionName);
    actionGroup->addAction(act);
    act->setData(QVariant::fromValue(service));
    return act;
}

void KTNEFMain::createOpenWithMenu(QMenu *topMenu)
{
    if (mView->getSelection().isEmpty()) {
        return;
    }
    KTNEFAttach *attach = mView->getSelection().at(0);
    const QString mimename(attach->mimeTag());
    if (const KService::List offers = KFileItemActions::associatedApplications(QStringList() << mimename); !offers.isEmpty()) {
        QMenu *menu = topMenu;
        auto actionGroup = new QActionGroup(menu);
        connect(actionGroup, &QActionGroup::triggered, this, &KTNEFMain::slotOpenWithAction);

        if (offers.count() > 1) { // submenu 'open with'
            menu = new QMenu(i18nc("@title:menu", "&Open With"), topMenu);
            menu->menuAction()->setObjectName("openWith_submenu"_L1); // for the unittest
            topMenu->addMenu(menu);
        }
        for (const auto &s : offers) {
            QAction *act = createAppAction(s,
                                           // no submenu -> prefix single offer
                                           menu == topMenu,
                                           actionGroup,
                                           menu);
            menu->addAction(act);
        }

        QString openWithActionName;
        if (menu != topMenu) { // submenu
            menu->addSeparator();
            openWithActionName = i18nc("@action:inmenu Open With", "&Other…");
        } else {
            openWithActionName = i18nc("@title:menu", "&Open With…");
        }
        auto openWithAct = new QAction(menu);
        openWithAct->setText(openWithActionName);
        connect(openWithAct, &QAction::triggered, this, &KTNEFMain::viewFileAs);
        menu->addAction(openWithAct);
    } else { // no app offers -> Open With...
        auto act = new QAction(topMenu);
        act->setText(i18nc("@title:menu", "&Open With…"));
        connect(act, &QAction::triggered, this, &KTNEFMain::viewFileAs);
        topMenu->addAction(act);
    }
}

void KTNEFMain::slotOpenWithAction(QAction *act)
{
    auto app = act->data().value<KService::Ptr>();

    openWith(app);
}

#include "moc_ktnefmain.cpp"
