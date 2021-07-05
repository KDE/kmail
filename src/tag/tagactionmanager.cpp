/*
   SPDX-FileCopyrightText: 2010 Thomas McGuire <mcguire@kde.org>
   SPDX-FileCopyrightText: 2011-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/
#include "tagactionmanager.h"
#include "tagmonitormanager.h"

#include "messageactions.h"

#include <MailCommon/AddTagDialog>

#include "kmail_debug.h"
#include <AkonadiCore/Monitor>
#include <KActionCollection>
#include <KActionMenu>
#include <KLocalizedString>
#include <KToggleAction>
#include <KXMLGUIClient>
#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QPointer>

using namespace KMail;

static int s_numberMaxTag = 10;

TagActionManager::TagActionManager(QObject *parent, KActionCollection *actionCollection, MessageActions *messageActions, KXMLGUIClient *guiClient)
    : QObject(parent)
    , mActionCollection(actionCollection)
    , mMessageActions(messageActions)
    , mGUIClient(guiClient)
{
    mMessageActions->messageStatusMenu()->menu()->addSeparator();

    TagMonitorManager *tagMonitorManager = TagMonitorManager::self();
    connect(tagMonitorManager, &TagMonitorManager::tagAdded, this, &TagActionManager::fillTagList);
    connect(tagMonitorManager, &TagMonitorManager::tagRemoved, this, &TagActionManager::fillTagList);
    connect(tagMonitorManager, &TagMonitorManager::tagChanged, this, &TagActionManager::fillTagList);
    connect(tagMonitorManager, &TagMonitorManager::fetchTagDone, this, &TagActionManager::createActions);
}

TagActionManager::~TagActionManager() = default;

void TagActionManager::clearActions()
{
    // Remove the tag actions from the toolbar
    if (!mToolbarActions.isEmpty()) {
        if (mGUIClient->factory()) {
            mGUIClient->unplugActionList(QStringLiteral("toolbar_messagetag_actions"));
        }
        mToolbarActions.clear();
    }

    // Remove the tag actions from the status menu and the action collection,
    // then delete them.
    for (KToggleAction *action : std::as_const(mTagActions)) {
        mMessageActions->messageStatusMenu()->removeAction(action);

        // This removes and deletes the action at the same time
        mActionCollection->removeAction(action);
    }

    if (mSeparatorMoreAction) {
        mMessageActions->messageStatusMenu()->removeAction(mSeparatorMoreAction);
    }

    if (mSeparatorNewTagAction) {
        mMessageActions->messageStatusMenu()->removeAction(mSeparatorNewTagAction);
    }

    if (mNewTagAction) {
        mMessageActions->messageStatusMenu()->removeAction(mNewTagAction);
    }

    if (mMoreAction) {
        mMessageActions->messageStatusMenu()->removeAction(mMoreAction);
    }

    mTagActions.clear();
}

void TagActionManager::createTagAction(const MailCommon::Tag::Ptr &tag, bool addToMenu)
{
    QString cleanName(i18n("Message Tag: %1", tag->tagName));
    cleanName.replace(QLatin1Char('&'), QStringLiteral("&&"));
    auto const tagAction = new KToggleAction(QIcon::fromTheme(tag->iconName), cleanName, this);
    tagAction->setIconText(tag->name());
    tagAction->setChecked(tag->id() == mNewTagId);

    mActionCollection->addAction(tag->name(), tagAction);
    mActionCollection->setDefaultShortcut(tagAction, QKeySequence(tag->shortcut));
    const QString tagName = QString::number(tag->tag().id());
    connect(tagAction, &KToggleAction::triggered, this, [this, tagName] {
        onSignalMapped(tagName);
    });

    // The shortcut configuration is done in the config dialog.
    // The shortcut set in the shortcut dialog would not be saved back to
    // the tag descriptions correctly.
    mActionCollection->setShortcutsConfigurable(tagAction, false);

    mTagActions.insert(tag->id(), tagAction);
    if (addToMenu) {
        mMessageActions->messageStatusMenu()->menu()->addAction(tagAction);
    }

    if (tag->inToolbar) {
        mToolbarActions.append(tagAction);
    }
}

void TagActionManager::createActions()
{
    createTagActions(TagMonitorManager::self()->tags());
}

void TagActionManager::onSignalMapped(const QString &tag)
{
    Q_EMIT tagActionTriggered(Akonadi::Tag(tag.toLongLong()));
}

void TagActionManager::createTagActions(const QVector<MailCommon::Tag::Ptr> &tags)
{
    clearActions();

    // Create a action for each tag and plug it into various places
    int i = 0;
    bool needToAddMoreAction = false;
    const int numberOfTag(tags.size());
    // It is assumed the tags are sorted
    for (const MailCommon::Tag::Ptr &tag : tags) {
        if (i < s_numberMaxTag) {
            createTagAction(tag, true);
        } else {
            if (tag->inToolbar || !tag->shortcut.isEmpty()) {
                createTagAction(tag, false);
            }

            if (i == s_numberMaxTag && i < numberOfTag) {
                needToAddMoreAction = true;
            }
        }
        ++i;
    }

    if (!mSeparatorNewTagAction) {
        mSeparatorNewTagAction = new QAction(this);
        mSeparatorNewTagAction->setSeparator(true);
    }
    mMessageActions->messageStatusMenu()->menu()->addAction(mSeparatorNewTagAction);

    if (!mNewTagAction) {
        mNewTagAction = new QAction(QIcon::fromTheme(QStringLiteral("tag-new")), i18n("Add new tag..."), this);
        connect(mNewTagAction, &QAction::triggered, this, &TagActionManager::newTagActionClicked);
    }
    mMessageActions->messageStatusMenu()->menu()->addAction(mNewTagAction);

    if (needToAddMoreAction) {
        if (!mSeparatorMoreAction) {
            mSeparatorMoreAction = new QAction(this);
            mSeparatorMoreAction->setSeparator(true);
        }
        mMessageActions->messageStatusMenu()->menu()->addAction(mSeparatorMoreAction);

        if (!mMoreAction) {
            mMoreAction = new QAction(i18n("More..."), this);
            connect(mMoreAction, &QAction::triggered, this, &TagActionManager::tagMoreActionClicked);
        }
        mMessageActions->messageStatusMenu()->menu()->addAction(mMoreAction);
    }

    if (!mToolbarActions.isEmpty() && mGUIClient->factory()) {
        mGUIClient->plugActionList(QStringLiteral("toolbar_messagetag_actions"), mToolbarActions);
    }
}

void TagActionManager::updateActionStates(int numberOfSelectedMessages, const Akonadi::Item &selectedItem)
{
    mNewTagId = -1;
    QMap<qint64, KToggleAction *>::const_iterator it = mTagActions.constBegin();
    QMap<qint64, KToggleAction *>::const_iterator end = mTagActions.constEnd();
    if (numberOfSelectedMessages >= 1) {
        Q_ASSERT(selectedItem.isValid());
        for (; it != end; ++it) {
            // FIXME Not very performant tag label retrieval
            QString label(i18n("Tag not Found"));
            const auto tags = TagMonitorManager::self()->tags();
            for (const MailCommon::Tag::Ptr &tag : tags) {
                if (tag->id() == it.key()) {
                    label = tag->name();
                    break;
                }
            }

            it.value()->setEnabled(true);
            if (numberOfSelectedMessages == 1) {
                const bool hasTag = selectedItem.hasTag(Akonadi::Tag(it.key()));
                it.value()->setChecked(hasTag);
                it.value()->setText(i18n("Message Tag: %1", label));
            } else {
                it.value()->setChecked(false);
                it.value()->setText(i18n("Toggle Message Tag: %1", label));
            }
        }
    } else {
        for (; it != end; ++it) {
            it.value()->setEnabled(false);
        }
    }
}

void TagActionManager::fillTagList()
{
    const QList<qint64> checked = checkedTags();

    clearActions();
    createTagActions(TagMonitorManager::self()->tags());

    checkTags(checked);
}

void TagActionManager::newTagActionClicked()
{
    QPointer<MailCommon::AddTagDialog> dialog = new MailCommon::AddTagDialog(QList<KActionCollection *>() << mActionCollection, nullptr);
    dialog->setTags(TagMonitorManager::self()->tags());
    if (dialog->exec() == QDialog::Accepted) {
        mNewTagId = dialog->tag().id();
        // Assign tag to all selected items right away
        Q_EMIT tagActionTriggered(dialog->tag());
    }
    delete dialog;
}

void TagActionManager::checkTags(const QList<qint64> &tags)
{
    for (const qint64 id : tags) {
        if (mTagActions.contains(id)) {
            mTagActions[id]->setChecked(true);
        }
    }
}

QList<qint64> TagActionManager::checkedTags() const
{
    QMap<qint64, KToggleAction *>::const_iterator it = mTagActions.constBegin();
    QMap<qint64, KToggleAction *>::const_iterator end = mTagActions.constEnd();
    QList<qint64> checked;
    for (; it != end; ++it) {
        if (it.value()->isChecked()) {
            checked << it.key();
        }
    }
    return checked;
}
