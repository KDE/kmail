/*
    encodedimagepicker.cpp

    KMail, the KDE mail client.
    SPDX-FileCopyrightText: 2021 the KMail authors.
    See file AUTHORS for details

    SPDX-License-Identifier: GPL-2.0-only
*/

#include "encodedimagepicker.h"
#include "ui_encodedimagepicker.h"

#include <Akonadi/ContactSearchJob>
#include <KIdentityManagementCore/Identity>
#include <KIdentityManagementCore/IdentityManager>
#include <TextCustomEditor/PlainTextEditor>

#include <KIO/StoredTransferJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>

#include <QFileDialog>
#include <QImageReader>

using namespace KMail;
using KContacts::Addressee;

EncodedImagePicker::EncodedImagePicker(QWidget *parent)
    : QGroupBox(parent)
    , mUi(new Ui::EncodedImagePicker)
{
    mUi->setupUi(this);

    mUi->source->editor()->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    // wrap anywhere, these contain encoded data
    mUi->source->editor()->setWordWrapMode(QTextOption::WrapAnywhere);

    connect(mUi->openImageButton, &QAbstractButton::clicked, this, &EncodedImagePicker::selectFile);
    connect(mUi->selectContactsButton, &QPushButton::released, this, &EncodedImagePicker::selectFromAddressBook);
    connect(mUi->source->editor(), &TextCustomEditor::PlainTextEditor::textChanged, this, &EncodedImagePicker::sourceChanged);
}

EncodedImagePicker::~EncodedImagePicker() = default;

void EncodedImagePicker::setInfo(const QString &info)
{
    mUi->infoLabel->setText(info);
}

QString EncodedImagePicker::source() const
{
    return mUi->source->editor()->toPlainText();
}

void EncodedImagePicker::setSource(const QString &source)
{
    mUi->source->editor()->setPlainText(source);
}

void EncodedImagePicker::setImage(const QImage &image)
{
    if (image.isNull()) {
        mUi->image->clear();
    } else {
        const QPixmap p = QPixmap::fromImage(image);
        mUi->image->setPixmap(p);
    }
}

void EncodedImagePicker::selectFile()
{
    QString filter;
    const QList<QByteArray> supportedImage = QImageReader::supportedImageFormats();
    for (const QByteArray &ba : supportedImage) {
        if (!filter.isEmpty()) {
            filter += QLatin1Char(' ');
        }
        filter += QLatin1String("*.") + QString::fromLatin1(ba);
    }

    filter = QStringLiteral("%1 (%2)").arg(i18n("Image"), filter);
    const QUrl url = QFileDialog::getOpenFileUrl(this, QString(), QUrl(), filter);

    if (!url.isEmpty()) {
        setFromFile(url);
    }
}

void EncodedImagePicker::setFromFile(const QUrl &url)
{
    auto job = KIO::storedGet(url);
    KJobWidgets::setWindow(job, this);
    connect(job, &KJob::result, this, &EncodedImagePicker::setFromFileDone);
    job->start();
}

void EncodedImagePicker::setFromFileDone(KJob *job)
{
    const KIO::StoredTransferJob *kioJob = qobject_cast<KIO::StoredTransferJob *>(job);

    if (kioJob->error() == 0) {
        const QImage image = QImage::fromData(kioJob->data());

        Q_EMIT imageSelected(image);
    } else {
        KMessageBox::error(this, kioJob->errorString());
    }
}

void EncodedImagePicker::selectFromAddressBook()
{
    using namespace KIdentityManagementCore;

    const IdentityManager manager(true);
    const Identity defaultIdentity = manager.defaultIdentity();
    const QString email = defaultIdentity.primaryEmailAddress().toLower();

    auto job = new Akonadi::ContactSearchJob(this);
    job->setLimit(1);
    job->setQuery(Akonadi::ContactSearchJob::Email, email, Akonadi::ContactSearchJob::ExactMatch);
    connect(job, &KJob::result, this, &EncodedImagePicker::selectFromAddressBookDone);
}

void EncodedImagePicker::selectFromAddressBookDone(KJob *job)
{
    const Akonadi::ContactSearchJob *searchJob = qobject_cast<Akonadi::ContactSearchJob *>(job);

    if (searchJob->contacts().isEmpty()) {
        KMessageBox::information(this, i18n("You do not have your own contact defined in the address book."), i18nc("@title:window", "No Picture"));
        return;
    }

    const Addressee contact = searchJob->contacts().at(0);

    if (contact.photo().isIntern()) {
        const QImage photo = contact.photo().data();

        if (photo.isNull()) {
            KMessageBox::information(this, i18n("No picture set for your address book entry."), i18nc("@title:window", "No Picture"));
        } else {
            Q_EMIT imageSelected(photo);
        }
    } else {
        const QUrl url(contact.photo().url());

        if (url.isEmpty()) {
            KMessageBox::information(this, i18n("No picture set for your address book entry."), i18nc("@title:window", "No Picture"));
        } else {
            setFromFile(url);
        }
    }
}

#include "moc_encodedimagepicker.cpp"
