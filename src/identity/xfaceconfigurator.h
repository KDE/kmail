/*  -*- c++ -*-
    xfaceconfigurator.cpp

    KMail, the KDE mail client.
    SPDX-FileCopyrightText: 2004 Jakob Schröter <js@camaya.net>
    SPDX-FileCopyrightText: 2002 the KMail authors.
    See file AUTHORS for details

    SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <QWidget>

class KJob;
class QUrl;

class QCheckBox;
class QLabel;
namespace KPIMTextEdit
{
class PlainTextEditorWidget;
}
namespace KMail
{
class XFaceConfigurator : public QWidget
{
    Q_OBJECT
public:
    explicit XFaceConfigurator(QWidget *parent = nullptr);
    ~XFaceConfigurator() override;

    Q_REQUIRED_RESULT bool isXFaceEnabled() const;
    void setXFaceEnabled(bool enable);

    Q_REQUIRED_RESULT QString xface() const;
    void setXFace(const QString &text);

private:
    void setXfaceFromFile(const QUrl &url);

    void slotSelectFile();
    void slotSelectFromAddressbook();
    void slotDelayedSelectFromAddressbook(KJob *);
    void slotUpdateXFace();

    QCheckBox *const mEnableCheck;
    KPIMTextEdit::PlainTextEditorWidget *mTextEdit = nullptr;
    QLabel *const mXFaceLabel;
};
} // namespace KMail

