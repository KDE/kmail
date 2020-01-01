/*
   Copyright 2010 Thomas McGuire <mcguire@kde.org>
   Copyright 2011-2020 Laurent Montel <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "tagactionmanager.h"

#include "messageactions.h"

#include "MailCommon/AddTagDialog"

#include <QAction>
#include <KActionCollection>
#include <KToggleAction>
#include <KXMLGUIClient>
#include <KActionMenu>
#include <QMenu>
#include <KLocalizedString>
#include <KJob>
#include <QIcon>
#include <AkonadiCore/Monitor>
#include "kmail_debug.h"

#include <QPointer>

#include <AkonadiCore/TagFetchJob>
#include <AkonadiCore/TagFetchScope>
#include <AkonadiCore/TagAttribute>

using namespace KMail;

static int s_numberMaxTag = 10;

TagActionManager::TagActionManager(QObject *parent, KActionCollection *actionCollection, MessageActions *messageActions, KXMLGUIClient *guiClient)
    : QObject(parent)
    , mActionCollection(actionCollection)
    , mMessageActions(messageActions)
    , mGUIClient(guiClient)
    , mMonitor(new Akonadi::Monitor(this))
{
    mMessageActions->messageStatusMenu()->menu()->addSeparator();

    mMonitor->setObjectName(QStringLiteral("TagActionManagerMonitor"));
    mMonitor->setTypeMonitored(Akonadi::Monitor::Tags);
    mMonitor->tagFetchScope().fetchAttribute<Akonadi::TagAttribute>();
    connect(mMonitor, &Akonadi::Monitor::tagAdded, this, &TagActionManager::onTagAdded);
    connect(mMonitor, &Akonadi::Monitor::tagRemoved, this, &TagActionManager::onTagRemoved);
    connect(mMonitor, &Akonadi::Monitor::tagChanged, this, &TagActionManager::onTagChanged);
}

TagActionManager::~TagActionManager()
{
}

void TagActionManager::clearActions()
{
    //Remove the tag actions from the toolbar
    if (!mToolbarActions.isEmpty()) {
        if (mGUIClient->factory()) {
            mGUIClient->unplugActionList(QStringLiteral("toolbar_messagetag_actions"));
        }
        mToolbarActions.clear();
    }

    //Remove the tag actions from the status menu and the action collection,
    //then delete them.
    for (KToggleAction *action : qAsConst(mTagActions)) {
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
    KToggleAction *const tagAction = new KToggleAction(QIcon::fromTheme(tag->iconName),
                                                       cleanName, this);
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
    if (mTagFetchInProgress) {
        return;
    }

    if (mTags.isEmpty()) {
        mTagFetchInProgress = true;
        Akonadi::TagFetchJob *fetchJob = new Akonadi::TagFetchJob(this);
        fetchJob->fetchScope().fetchAttribute<Akonadi::TagAttribute>();
        connect(fetchJob, &Akonadi::TagFetchJob::result, this, &TagActionManager::finishedTagListing);
    } else {
        mTagFetchInProgress = false;
        createTagActions(mTags);
    }
}

void TagActionManager::finishedTagListing(KJob *job)
{
    if (job->error()) {
        qCWarning(KMAIL_LOG) << job->errorString();
    }
    Akonadi::TagFetchJob *fetchJob = static_cast<Akonadi::TagFetchJob *>(job);
    const Akonadi::Tag::List lstTags = fetchJob->tags();
    for (const Akonadi::Tag &result : lstTags) {
        mTags.append(MailCommon::Tag::fromAkonadi(result));
    }
    mTagFetchInProgress = false;
    std::sort(mTags.begin(), mTags.end(), MailCommon::Tag::compare);
    createTagActions(mTags);
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
    //It is assumed the tags are sorted
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
        mNewTagAction = new QAction(i18n("Add new tag..."), this);
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
            connect(mMoreAction, &QAction::triggered,
                    this, &TagActionManager::tagMoreActionClicked);
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
            //FIXME Not very performant tag label retrieval
            QString label(i18n("Tag not Found"));
            for (const MailCommon::Tag::Ptr &tag : qAsConst(mTags)) {
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

void TagActionManager::onTagAdded(const Akonadi::Tag &akonadiTag)
{
    const QList<qint64> checked = checkedTags();

    clearActions();
    mTags.append(MailCommon::Tag::fromAkonadi(akonadiTag));
    std::sort(mTags.begin(), mTags.end(), MailCommon::Tag::compare);
    createTagActions(mTags);

    checkTags(checked);
}

void TagActionManager::onTagRemoved(const Akonadi::Tag &akonadiTag)
{
    for (const MailCommon::Tag::Ptr &tag : qAsConst(mTags)) {
        if (tag->id() == akonadiTag.id()) {
            mTags.removeAll(tag);
            break;
        }
    }

    fillTagList();
}

void TagActionManager::onTagChanged(const Akonadi::Tag &akonadiTag)
{
    for (const MailCommon::Tag::Ptr &tag : qAsConst(mTags)) {
        if (tag->id() == akonadiTag.id()) {
            mTags.removeAll(tag);
            break;
        }
    }
    mTags.append(MailCommon::Tag::fromAkonadi(akonadiTag));
    fillTagList();
}

void TagActionManager::fillTagList()
{
    const QList<qint64> checked = checkedTags();

    clearActions();
    std::sort(mTags.begin(), mTags.end(), MailCommon::Tag::compare);
    createTagActions(mTags);

    checkTags(checked);
}

void TagActionManager::newTagActionClicked()
{
    QPointer<MailCommon::AddTagDialog> dialog = new MailCommon::AddTagDialog(QList<KActionCollection *>() << mActionCollection, nullptr);
    dialog->setTags(mTags);
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
