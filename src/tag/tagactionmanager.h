/* SPDX-FileCopyrightText: 2010 Thomas McGuire <mcguire@kde.org>

   SPDX-FileCopyrightText: 2011-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/
#pragma once

#include "kmail_export.h"
#include "mailcommon/tag.h"
#include <AkonadiCore/Item>
#include <QMap>
#include <QVector>
class KActionCollection;
class KXMLGUIClient;
class KToggleAction;
class QAction;
namespace Akonadi
{
class Item;
class Tag;
}

namespace KMail
{
class MessageActions;

/**
 * Creates actions related to the existing Akonadi tags and plugs them into the GUI.
 *
 * The tag manager reads all tags from Akonadi and adds each to the action collection
 * and to the message status menu.
 * For tags that should be in the toolbar, it plugs the action list
 * toolbar_messagetag_actions.
 *
 * The actions are automatically updated when a Akonadi tag changes.
 */
class KMAIL_EXPORT TagActionManager : public QObject
{
    Q_OBJECT
public:
    /**
     * Does not take ownership of the action collection, the GUI client or the message actions.
     * Does not yet create the actions.
     *
     * @param parent The parent QObject.
     * @param actionCollection: Each tag action is added here
     * @param messageActions: Each action is added to the message status menu
     * @param guiClient: The action list with the toolbar action is plugged here
     */
    TagActionManager(QObject *parent, KActionCollection *actionCollection, MessageActions *messageActions, KXMLGUIClient *guiClient);

    ~TagActionManager() override;

    /**
     * Removes all actions from the GUI again
     */
    void clearActions();

    /**
     * Creates and plugs all tag actions
     */
    void createActions();

    /**
     * Updates the state of the toggle actions of all tags.
     * The state of the action depends on the number of selected messages, for example
     * all actions are disabled when no message is selected.
     *
     * This function is async
     *
     * @param numberOfSelectedMessages The number of selected messages
     * @param selectedItem if exactly one item is selected, it should be passed here
     */
    void updateActionStates(int numberOfSelectedMessages, const Akonadi::Item &selectedItem);

Q_SIGNALS:

    /**
     * Emitted when one of the tagging actions was triggered. The user of this class
     * should connect to this signal and change the tags of the messages
     */
    void tagActionTriggered(const Akonadi::Tag &tag);
    /**
     * Emitted when we want to select more action
     */
    void tagMoreActionClicked();

private:
    Q_DISABLE_COPY(TagActionManager)
    void newTagActionClicked();
    void onSignalMapped(const QString &tag);

    void fillTagList();
    void createTagAction(const MailCommon::Tag::Ptr &tag, bool addToMenu);
    void createTagActions(const QVector<MailCommon::Tag::Ptr> &);
    void checkTags(const QList<qint64> &tags);
    Q_REQUIRED_RESULT QList<qint64> checkedTags() const;

    KActionCollection *const mActionCollection;
    MessageActions *const mMessageActions;
    KXMLGUIClient *const mGUIClient;

    QAction *mSeparatorMoreAction = nullptr;
    QAction *mSeparatorNewTagAction = nullptr;
    QAction *mMoreAction = nullptr;
    QAction *mNewTagAction = nullptr;
    // Maps the id of a tag to the action of a tag.
    // Contains all existing tags
    QMap<qint64, KToggleAction *> mTagActions;

    // The actions of all tags that are in the toolbar
    QList<QAction *> mToolbarActions;

    // Uri of a newly created tag
    qint64 mNewTagId = -1;
};
}

