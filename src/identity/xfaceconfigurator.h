/*  -*- c++ -*-
    xfaceconfigurator.cpp

    KMail, the KDE mail client.
    SPDX-FileCopyrightText: 2004 Jakob Schröter <js@camaya.net>
    SPDX-FileCopyrightText: 2002 the KMail authors.
    See file AUTHORS for details

    SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <QProcess>
#include <QWidget>

namespace Ui
{
class XFaceConfigurator;
}

namespace KMail
{
class XFaceConfigurator : public QWidget
{
    Q_OBJECT
public:
    enum Mode {
        DontSend,
        SendFace,
        SendXFace,
        SendBoth,
    };
    Q_ENUM(Mode)

    explicit XFaceConfigurator(QWidget *parent = nullptr);
    ~XFaceConfigurator() override;

    Q_REQUIRED_RESULT bool isXFaceEnabled() const;
    void setXFaceEnabled(bool enable);

    Q_REQUIRED_RESULT QString xface() const;
    void setXFace(const QString &text);

    Q_REQUIRED_RESULT bool isFaceEnabled() const;
    void setFaceEnabled(bool enable);

    Q_REQUIRED_RESULT QString face() const;
    void setFace(const QString &text);

private:
    void crunch(const QImage &image);
    Q_REQUIRED_RESULT bool pngquant(const QImage &image);

private Q_SLOTS:
    void modeChanged(int);

    void compressFace(const QImage &);
    void compressFaceDone(const QByteArray &, bool fromPngquant);
    void compressXFace(const QImage &);
    void updateFace();
    void updateXFace();

    void pngquantFinished(int, QProcess::ExitStatus);

private:
    QScopedPointer<Ui::XFaceConfigurator> mUi;
    QProcess *const mPngquantProc;
};
} // namespace KMail

