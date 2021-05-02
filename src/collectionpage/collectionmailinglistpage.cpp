/*
  This file is part of KMail, the KDE mail client.
  SPDX-FileCopyrightText: 2005 Till Adam <adam@kde.org>
  SPDX-FileCopyrightText: 2011-2021 Laurent Montel <montel@kde.org>
  SPDX-FileCopyrightText: 2012 Jonathan Marten <jjm@keelhaul.me.uk>

  SPDX-License-Identifier: GPL-2.0-only
*/

#include "collectionmailinglistpage.h"
#include "util.h"
#include <MailCommon/MailKernel>
#include <MailCommon/MailUtil>

#include <Akonadi/KMime/MessageParts>
#include <AkonadiCore/itemfetchjob.h>
#include <AkonadiCore/itemfetchscope.h>

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpacerItem>
#include <QVBoxLayout>

#include "kmail_debug.h"
#include <KEditListWidget>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSqueezedTextLabel>
#include <QComboBox>

using namespace MailCommon;

CollectionMailingListPage::CollectionMailingListPage(QWidget *parent)
    : CollectionPropertiesPage(parent)
{
    setObjectName(QStringLiteral("KMail::CollectionMailingListPage"));
    setPageTitle(i18nc("@title:tab Mailing list settings for a folder.", "Mailing List"));
}

CollectionMailingListPage::~CollectionMailingListPage() = default;

void CollectionMailingListPage::slotConfigChanged()
{
    changed = true;
}

bool CollectionMailingListPage::canHandle(const Akonadi::Collection &col) const
{
    QSharedPointer<FolderSettings> fd = FolderSettings::forCollection(col, false);
    return !CommonKernel->isSystemFolderCollection(col) && !fd->isStructural() && !MailCommon::Util::isVirtualCollection(col);
}

void CollectionMailingListPage::init(const Akonadi::Collection &col)
{
    mCurrentCollection = col;
    mFolder = FolderSettings::forCollection(col, false);

    auto topLayout = new QVBoxLayout(this);

    mHoldsMailingList = new QCheckBox(i18n("Folder holds a mailing list"), this);
    connect(mHoldsMailingList, &QCheckBox::toggled, this, &CollectionMailingListPage::slotHoldsML);
    connect(mHoldsMailingList, &QCheckBox::toggled, this, &CollectionMailingListPage::slotConfigChanged);
    topLayout->addWidget(mHoldsMailingList);

    mGroupWidget = new QWidget(this);
    auto groupLayout = new QGridLayout(mGroupWidget);

    mDetectButton = new QPushButton(i18n("Detect Automatically"), mGroupWidget);
    connect(mDetectButton, &QPushButton::pressed, this, &CollectionMailingListPage::slotDetectMailingList);
    groupLayout->addWidget(mDetectButton, 2, 1);

    groupLayout->addItem(new QSpacerItem(0, 10), 3, 0);

    auto label = new QLabel(i18n("Mailing list description:"), mGroupWidget);
    groupLayout->addWidget(label, 4, 0);
    mMLId = new KSqueezedTextLabel(QString(), mGroupWidget);
    mMLId->setTextElideMode(Qt::ElideRight);
    groupLayout->addWidget(mMLId, 4, 1, 1, 2);

    // FIXME: add QWhatsThis
    label = new QLabel(i18n("Preferred handler:"), mGroupWidget);
    groupLayout->addWidget(label, 5, 0);
    mMLHandlerCombo = new QComboBox(mGroupWidget);
    mMLHandlerCombo->addItem(i18n("KMail"), MailingList::KMail);
    mMLHandlerCombo->addItem(i18n("Browser"), MailingList::Browser);
    groupLayout->addWidget(mMLHandlerCombo, 5, 1, 1, 2);
    connect(mMLHandlerCombo, qOverload<int>(&QComboBox::activated), this, &CollectionMailingListPage::slotMLHandling);
    label->setBuddy(mMLHandlerCombo);

    label = new QLabel(i18n("Address type:"), mGroupWidget);
    groupLayout->addWidget(label, 6, 0);
    mAddressCombo = new QComboBox(mGroupWidget);
    label->setBuddy(mAddressCombo);
    groupLayout->addWidget(mAddressCombo, 6, 1);

    // FIXME: if the mailing list actions have either QAction's or toolbar buttons
    //       associated with them - remove this button since it's really silly
    //       here
    auto handleButton = new QPushButton(i18n("Invoke Handler"), mGroupWidget);
    if (mFolder) {
        connect(handleButton, &QPushButton::clicked, this, &CollectionMailingListPage::slotInvokeHandler);
    } else {
        handleButton->setEnabled(false);
    }

    groupLayout->addWidget(handleButton, 6, 2);

    mEditList = new KEditListWidget(mGroupWidget);
    mEditList->lineEdit()->setClearButtonEnabled(true);
    connect(mEditList, &KEditListWidget::changed, this, &CollectionMailingListPage::slotConfigChanged);
    groupLayout->addWidget(mEditList, 7, 0, 1, 4);

    // Order is important because the activate handler and fillMLFromWidgets
    // depend on it
    const QStringList el{i18n("Post to List"), i18n("Subscribe to List"), i18n("Unsubscribe From List"), i18n("List Archives"), i18n("List Help")};
    mAddressCombo->addItems(el);
    connect(mAddressCombo, qOverload<int>(&QComboBox::activated), this, &CollectionMailingListPage::slotAddressChanged);

    topLayout->addWidget(mGroupWidget);
    mGroupWidget->setEnabled(false);
}

void CollectionMailingListPage::load(const Akonadi::Collection &col)
{
    init(col);

    if (mFolder) {
        mMailingList = mFolder->mailingList();
    }

    mMLId->setText((mMailingList.id().isEmpty() ? i18n("Not available") : mMailingList.id()));
    mMLHandlerCombo->setCurrentIndex(mMailingList.handler());
    mEditList->insertStringList(QUrl::toStringList(mMailingList.postUrls()));

    mAddressCombo->setCurrentIndex(mLastItem);
    mHoldsMailingList->setChecked(mFolder && mFolder->isMailingListEnabled());
    slotHoldsML(mHoldsMailingList->isChecked());
    changed = false;
}

void CollectionMailingListPage::save(Akonadi::Collection &col)
{
    Q_UNUSED(col)
    if (changed) {
        if (mFolder) {
            // settings for mailingList
            mFolder->setMailingListEnabled(mHoldsMailingList && mHoldsMailingList->isChecked());
            fillMLFromWidgets();
            mFolder->setMailingList(mMailingList);
        }
    }
}

//----------------------------------------------------------------------------
void CollectionMailingListPage::slotHoldsML(bool holdsML)
{
    mGroupWidget->setEnabled(holdsML);
    mDetectButton->setEnabled(mFolder && mFolder->count() != 0);
}

//----------------------------------------------------------------------------
void CollectionMailingListPage::slotDetectMailingList()
{
    if (!mFolder) {
        return; // in case the folder was just created
    }

    qCDebug(KMAIL_LOG) << "Detecting mailing list";

    // next try the 5 most recently added messages
    if (!(mMailingList.features() & MailingList::Post)) {
        // FIXME not load all folder
        auto job = new Akonadi::ItemFetchJob(mCurrentCollection, this);
        job->fetchScope().fetchPayloadPart(Akonadi::MessagePart::Header);
        connect(job, &Akonadi::ItemFetchJob::result, this, &CollectionMailingListPage::slotFetchDone);
        // Don't allow to reactive it
        mDetectButton->setEnabled(false);
    } else {
        mMLId->setText((mMailingList.id().isEmpty() ? i18n("Not available.") : mMailingList.id()));
        fillEditBox();
    }
}

void CollectionMailingListPage::slotFetchDone(KJob *job)
{
    mDetectButton->setEnabled(true);
    if (MailCommon::Util::showJobErrorMessage(job)) {
        return;
    }
    auto fjob = qobject_cast<Akonadi::ItemFetchJob *>(job);
    Q_ASSERT(fjob);
    Akonadi::Item::List items = fjob->items();
    const int maxchecks = 5;
    int num = items.size();
    for (int i = --num; (i > num - maxchecks) && (i >= 0); --i) {
        Akonadi::Item item = items[i];
        if (item.hasPayload<KMime::Message::Ptr>()) {
            auto message = item.payload<KMime::Message::Ptr>();
            mMailingList = MessageCore::MailingList::detect(message);
            if (mMailingList.features() & MailingList::Post) {
                break;
            }
        }
    }
    if (!(mMailingList.features() & MailingList::Post)) {
        if (mMailingList.features() == MailingList::None) {
            KMessageBox::error(this, i18n("KMail was unable to detect any mailing list in this folder."));
        } else {
            KMessageBox::error(this,
                               i18n("KMail was unable to fully detect a mailing list in this folder. "
                                    "Please fill in the addresses by hand."));
        }
    } else {
        mMLId->setText((mMailingList.id().isEmpty() ? i18n("Not available.") : mMailingList.id()));
        fillEditBox();
    }
}

//----------------------------------------------------------------------------
void CollectionMailingListPage::slotMLHandling(int element)
{
    mMailingList.setHandler(static_cast<MailingList::Handler>(element));
    slotConfigChanged();
}

//----------------------------------------------------------------------------
void CollectionMailingListPage::slotAddressChanged(int i)
{
    fillMLFromWidgets();
    fillEditBox();
    mLastItem = i;
    slotConfigChanged();
}

//----------------------------------------------------------------------------
void CollectionMailingListPage::fillMLFromWidgets()
{
    if (!mHoldsMailingList->isChecked()) {
        return;
    }

    // make sure that email addresses are prepended by "mailto:"
    bool listChanged = false;
    const QStringList oldList = mEditList->items();
    QStringList newList; // the correct string list
    QStringList::ConstIterator end = oldList.constEnd();
    for (QStringList::ConstIterator it = oldList.constBegin(); it != end; ++it) {
        if (!(*it).startsWith(QLatin1String("http:")) && !(*it).startsWith(QLatin1String("https:")) && !(*it).startsWith(QLatin1String("mailto:"))
            && ((*it).contains(QLatin1Char('@')))) {
            listChanged = true;
            newList << QStringLiteral("mailto:") + *it;
        } else {
            newList << *it;
        }
    }
    if (listChanged) {
        mEditList->clear();
        mEditList->insertStringList(newList);
    }

    // mMailingList.setHandler( static_cast<MailingList::Handler>( mMLHandlerCombo->currentIndex() ) );
    switch (mLastItem) {
    case 0:
        mMailingList.setPostUrls(QUrl::fromStringList(mEditList->items()));
        break;
    case 1:
        mMailingList.setSubscribeUrls(QUrl::fromStringList(mEditList->items()));
        break;
    case 2:
        mMailingList.setUnsubscribeUrls(QUrl::fromStringList(mEditList->items()));
        break;
    case 3:
        mMailingList.setArchiveUrls(QUrl::fromStringList(mEditList->items()));
        break;
    case 4:
        mMailingList.setHelpUrls(QUrl::fromStringList(mEditList->items()));
        break;
    default:
        qCWarning(KMAIL_LOG) << "Wrong entry in the mailing list entry combo!";
    }
}

void CollectionMailingListPage::fillEditBox()
{
    mEditList->clear();
    switch (mAddressCombo->currentIndex()) {
    case 0:
        mEditList->insertStringList(QUrl::toStringList(mMailingList.postUrls()));
        break;
    case 1:
        mEditList->insertStringList(QUrl::toStringList(mMailingList.subscribeUrls()));
        break;
    case 2:
        mEditList->insertStringList(QUrl::toStringList(mMailingList.unsubscribeUrls()));
        break;
    case 3:
        mEditList->insertStringList(QUrl::toStringList(mMailingList.archiveUrls()));
        break;
    case 4:
        mEditList->insertStringList(QUrl::toStringList(mMailingList.helpUrls()));
        break;
    default:
        qCWarning(KMAIL_LOG) << "Wrong entry in the mailing list entry combo!";
    }
}

void CollectionMailingListPage::slotInvokeHandler()
{
    save(mCurrentCollection);
    switch (mAddressCombo->currentIndex()) {
    case 0:
        if (!KMail::Util::mailingListPost(mFolder, mCurrentCollection)) {
            qCWarning(KMAIL_LOG) << "invalid folder";
        }
        break;
    case 1:
        if (!KMail::Util::mailingListSubscribe(mFolder, mCurrentCollection)) {
            qCWarning(KMAIL_LOG) << "invalid folder";
        }
        break;
    case 2:
        if (!KMail::Util::mailingListUnsubscribe(mFolder, mCurrentCollection)) {
            qCWarning(KMAIL_LOG) << "invalid folder";
        }
        break;
    case 3:
        if (!KMail::Util::mailingListArchives(mFolder, mCurrentCollection)) {
            qCWarning(KMAIL_LOG) << "invalid folder";
        }
        break;
    case 4:
        if (!KMail::Util::mailingListHelp(mFolder, mCurrentCollection)) {
            qCWarning(KMAIL_LOG) << "invalid folder";
        }
        break;
    default:
        qCWarning(KMAIL_LOG) << "Wrong entry in the mailing list entry combo!";
    }
}
