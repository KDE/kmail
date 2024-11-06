/*  -*- c++ -*-
    encodedimagepicker.h

    KMail, the KDE mail client.
    SPDX-FileCopyrightText: 2021 the KMail authors.
    See file AUTHORS for details

    SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <QGroupBox>
#include <memory>
class KJob;

namespace Ui
{
class EncodedImagePicker;
}

namespace KMail
{
class EncodedImagePicker : public QGroupBox
{
    Q_OBJECT
public:
    explicit EncodedImagePicker(QWidget *parent = nullptr);
    ~EncodedImagePicker() override;

    void setInfo(const QString &info);

    [[nodiscard]] QString source() const;
    void setSource(const QString &source);
    void setImage(const QImage &image);

Q_SIGNALS:
    void imageSelected(const QImage &);
    void sourceChanged();

private:
    void setFromFile(const QUrl &url);

    void selectFile();
    void setFromFileDone(KJob *);
    void selectFromAddressBook();
    void selectFromAddressBookDone(KJob *);
    std::unique_ptr<Ui::EncodedImagePicker> mUi;
};
} // namespace KMail
