/*
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

    [[nodiscard]] bool isXFaceEnabled() const;
    void setXFaceEnabled(bool enable);

    [[nodiscard]] QString xface() const;
    void setXFace(const QString &text);

    [[nodiscard]] bool isFaceEnabled() const;
    void setFaceEnabled(bool enable);

    [[nodiscard]] QString face() const;
    void setFace(const QString &text);

private:
    void crunch(const QImage &image);
    [[nodiscard]] bool pngquant(const QImage &image);

private:
    void modeChanged(int);

    void compressFace(const QImage &);
    void compressFaceDone(const QByteArray &, bool fromPngquant);
    void compressXFace(const QImage &);
    void updateFace();
    void updateXFace();

    void pngquantFinished(int, QProcess::ExitStatus);

    std::unique_ptr<Ui::XFaceConfigurator> mUi;
    QProcess *const mPngquantProc;
};
} // namespace KMail
