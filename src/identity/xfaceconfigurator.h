/*  -*- c++ -*-
    xfaceconfigurator.cpp

    KMail, the KDE mail client.
    Copyright (c) 2004 Jakob Schröter <js@camaya.net>
    Copyright (c) 2002 the KMail authors.
    See file AUTHORS for details

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#ifndef __KMAIL_XFACECONFIGURATOR_H__
#define __KMAIL_XFACECONFIGURATOR_H__

#include <QWidget>

class KJob;
class QUrl;

class QCheckBox;
class QLabel;
namespace KPIMTextEdit {
class PlainTextEditorWidget;
}
namespace KMail {
class XFaceConfigurator : public QWidget
{
    Q_OBJECT
public:
    explicit XFaceConfigurator(QWidget *parent = nullptr);
    ~XFaceConfigurator();

    bool isXFaceEnabled() const;
    void setXFaceEnabled(bool enable);

    QString xface() const;
    void setXFace(const QString &text);

private:
    void setXfaceFromFile(const QUrl &url);

private Q_SLOTS:
    void slotSelectFile();
    void slotSelectFromAddressbook();
    void slotDelayedSelectFromAddressbook(KJob *);
    void slotUpdateXFace();

private:
    QCheckBox *mEnableCheck;
    KPIMTextEdit::PlainTextEditorWidget *mTextEdit;
    QLabel *mXFaceLabel;
};
} // namespace KMail

#endif // __KMAIL_XFACECONFIGURATOR_H__
