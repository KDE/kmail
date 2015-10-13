/*
  This file is part of KMail, the KDE mail client.
  Copyright (c) 2005 Till Adam <adam@kde.org>
  Copyright (c) 2011-2015 Montel Laurent <montel@kde.org>
  Copyright (c) 2012 Jonathan Marten <jjm@keelhaul.me.uk>

  KMail is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  KMail is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "collectionmailinglistpage.h"
#include "kmkernel.h"
#include "mailcommon/mailkernel.h"
#include "mailcommon/mailutil.h"
#include "util.h"

#include <AkonadiCore/itemfetchjob.h>
#include <AkonadiCore/itemfetchscope.h>
#include <Akonadi/KMime/MessageParts>

#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QSpacerItem>
#include <QVBoxLayout>

#include <KComboBox>
#include <QDialog>
#include <KLineEdit>
#include <KEditListWidget>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSqueezedTextLabel>
#include "kmail_debug.h"

using namespace MailCommon;

CollectionMailingListPage::CollectionMailingListPage(QWidget *parent) :
    CollectionPropertiesPage(parent), mGroupWidget(Q_NULLPTR), mLastItem(0), changed(false)
{
    setObjectName(QStringLiteral("KMail::CollectionMailingListPage"));
    setPageTitle(i18nc("@title:tab Mailing list settings for a folder.", "Mailing List"));
}

CollectionMailingListPage::~CollectionMailingListPage()
{
}

void CollectionMailingListPage::slotConfigChanged()
{
    changed = true;
}

bool CollectionMailingListPage::canHandle(const Akonadi::Collection &col) const
{
    QSharedPointer<FolderCollection> fd = FolderCollection::forCollection(col, false);
    return (!CommonKernel->isSystemFolderCollection(col) &&
            !fd->isStructural() &&
            !MailCommon::Util::isVirtualCollection(col));
}

void CollectionMailingListPage::init(const Akonadi::Collection &col)
{
    mCurrentCollection = col;
    mFolder = FolderCollection::forCollection(col, false);

    QVBoxLayout *topLayout = new QVBoxLayout(this);

    mHoldsMailingList = new QCheckBox(i18n("Folder holds a mailing list"), this);
    connect(mHoldsMailingList, &QCheckBox::toggled, this, &CollectionMailingListPage::slotHoldsML);
    connect(mHoldsMailingList, &QCheckBox::toggled, this, &CollectionMailingListPage::slotConfigChanged);
    topLayout->addWidget(mHoldsMailingList);

    mGroupWidget = new QWidget(this);
    QGridLayout *groupLayout = new QGridLayout(mGroupWidget);

    mDetectButton = new QPushButton(i18n("Detect Automatically"), mGroupWidget);
    connect(mDetectButton, &QPushButton::pressed, this, &CollectionMailingListPage::slotDetectMailingList);
    groupLayout->addWidget(mDetectButton, 2, 1);

    groupLayout->addItem(new QSpacerItem(0, 10), 3, 0);

    QLabel *label = new QLabel(i18n("Mailing list description:"), mGroupWidget);
    groupLayout->addWidget(label, 4, 0);
    mMLId = new KSqueezedTextLabel(QString(), mGroupWidget);
    mMLId->setTextElideMode(Qt::ElideRight);
    groupLayout->addWidget(mMLId, 4, 1, 1, 2);

    //FIXME: add QWhatsThis
    label = new QLabel(i18n("Preferred handler:"), mGroupWidget);
    groupLayout->addWidget(label, 5, 0);
    mMLHandlerCombo = new KComboBox(mGroupWidget);
    mMLHandlerCombo->addItem(i18n("KMail"), MailingList::KMail);
    mMLHandlerCombo->addItem(i18n("Browser"), MailingList::Browser);
    groupLayout->addWidget(mMLHandlerCombo, 5, 1, 1, 2);
    connect(mMLHandlerCombo, static_cast<void (KComboBox::*)(int)>(&KComboBox::activated), this, &CollectionMailingListPage::slotMLHandling);
    label->setBuddy(mMLHandlerCombo);

    label = new QLabel(i18n("Address type:"), mGroupWidget);
    groupLayout->addWidget(label, 6, 0);
    mAddressCombo = new KComboBox(mGroupWidget);
    label->setBuddy(mAddressCombo);
    groupLayout->addWidget(mAddressCombo, 6, 1);

    //FIXME: if the mailing list actions have either QAction's or toolbar buttons
    //       associated with them - remove this button since it's really silly
    //       here
    QPushButton *handleButton = new QPushButton(i18n("Invoke Handler"), mGroupWidget);
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

    QStringList el;

    //Order is important because the activate handler and fillMLFromWidgets
    //depend on it
    el << i18n("Post to List")
       << i18n("Subscribe to List")
       << i18n("Unsubscribe From List")
       << i18n("List Archives")
       << i18n("List Help");
    mAddressCombo->addItems(el);
    connect(mAddressCombo, static_cast<void (KComboBox::*)(int)>(&KComboBox::activated), this, &CollectionMailingListPage::slotAddressChanged);

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
    Q_UNUSED(col);
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
        return;    // in case the folder was just created
    }

    qCDebug(KMAIL_LOG) << "Detecting mailing list";

    // next try the 5 most recently added messages
    if (!(mMailingList.features() & MailingList::Post)) {
        //FIXME not load all folder
        Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(mFolder->collection(), this);
        job->fetchScope().fetchPayloadPart(Akonadi::MessagePart::Header);
        connect(job, &Akonadi::ItemFetchJob::result, this, &CollectionMailingListPage::slotFetchDone);
        //Don't allow to reactive it
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
    Akonadi::ItemFetchJob *fjob = dynamic_cast<Akonadi::ItemFetchJob *>(job);
    Q_ASSERT(fjob);
    Akonadi::Item::List items = fjob->items();
    const int maxchecks = 5;
    int num = items.size();
    for (int i = --num; (i > num - maxchecks) && (i >= 0); --i) {
        Akonadi::Item item = items[i];
        if (item.hasPayload<KMime::Message::Ptr>()) {
            KMime::Message::Ptr message = item.payload<KMime::Message::Ptr>();
            mMailingList = MessageCore::MailingList::detect(message);
            if (mMailingList.features() & MailingList::Post) {
                break;
            }
        }
    }
    if (!(mMailingList.features() & MailingList::Post)) {
        if (mMailingList.features() == MailingList::None) {
            KMessageBox::error(this,
                               i18n("KMail was unable to detect any mailing list in this folder."));
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
        if (!(*it).startsWith(QStringLiteral("http:")) && !(*it).startsWith(QStringLiteral("https:")) &&
                !(*it).startsWith(QStringLiteral("mailto:")) && ((*it).contains(QLatin1Char('@')))) {
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

    //mMailingList.setHandler( static_cast<MailingList::Handler>( mMLHandlerCombo->currentIndex() ) );
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
        KMail::Util::mailingListPost(mFolder);
        break;
    case 1:
        KMail::Util::mailingListSubscribe(mFolder);
        break;
    case 2:
        KMail::Util::mailingListUnsubscribe(mFolder);
        break;
    case 3:
        KMail::Util::mailingListArchives(mFolder);
        break;
    case 4:
        KMail::Util::mailingListHelp(mFolder);
        break;
    default:
        qCWarning(KMAIL_LOG) << "Wrong entry in the mailing list entry combo!";
    }
}

