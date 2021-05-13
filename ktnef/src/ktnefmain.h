/*
  This file is part of KTnef.

  SPDX-FileCopyrightText: 2002 Michael Goffioul <kdeprint@swing.be>
  SPDX-FileCopyrightText: 2012 Allen Winter <winter@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
*/

#pragma once

#include <KService>
#include <KXmlGuiWindow>
class QActionGroup;
class QAction;
class QContextMenuEvent;
class QTreeWidgetItem;
class KRecentFilesMenu;
class QUrl;

namespace KTnef
{
class KTNEFParser;
class KTNEFAttach;
}
using namespace KTnef;

class KTNEFView;

class KTNEFMain : public KXmlGuiWindow
{
    Q_OBJECT

public:
    explicit KTNEFMain(QWidget *parent = nullptr);
    ~KTNEFMain() override;

    void loadFile(const QString &filename);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

protected Q_SLOTS:
    void openFile();
    void viewFile();
    void viewFileAs();
    void extractFile();
    void extractFileTo();
    void propertiesFile();
    void optionDefaultDir();
    void extractAllFiles();
    void slotEditToolbars();
    void slotNewToolbarConfig();
    void slotShowMessageProperties();
    void slotShowMessageText();
    void slotSaveMessageText();

    void viewSelectionChanged();
    void viewDoubleClicked(QTreeWidgetItem *);
    void viewDragRequested(const QList<KTnef::KTNEFAttach *> &list);
    void slotConfigureKeys();
    void openRecentFile(const QUrl &);

private:
    void slotOpenWithAction(QAction *act);
    void addRecentFile(const QUrl &url);
    void setupStatusbar();
    void setupActions();
    void setupTNEF();
    void enableExtractAll(bool on = true);
    void enableSingleAction(bool on = true);
    void cleanup();

    void extractTo(const QString &dirname);
    QString extractTemp(KTNEFAttach *att);

    void openWith(const KService::Ptr &offer);
    void createOpenWithMenu(QMenu *topMenu);
    QAction *createAppAction(const KService::Ptr &service, bool singleOffer, QActionGroup *actionGroup, QObject *parent);

private:
    QString mFilename;
    QString mDefaultDir;
    QString mLastDir;
    KTNEFView *mView = nullptr;
    KTNEFParser *mParser = nullptr;
    KRecentFilesMenu *mOpenRecentFileMenu = nullptr;
};
Q_DECLARE_METATYPE(KService::Ptr)
