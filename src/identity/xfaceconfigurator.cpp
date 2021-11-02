/*
    This file is part of KMail.

    SPDX-FileCopyrightText: 2004 Jakob Schröter <js@camaya.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "xfaceconfigurator.h"
#include "encodedimagepicker.h"
#include "ui_xfaceconfigurator.h"

#include <MessageViewer/KXFace>

#include <KLocalizedString>
#include <KMessageBox>

#include <QBuffer>

using namespace KMail;
using MessageViewer::KXFace;

// The size of the PNG used in the Face header must be at most 725 bytes, as
// explained here: https://quimby.gnus.org/circus/face/
#define FACE_MAX_SIZE 725

XFaceConfigurator::XFaceConfigurator(QWidget *parent)
    : QWidget(parent)
    , mUi(new Ui::XFaceConfigurator)
    , mPngquantProc(new QProcess(this))
{
    mUi->setupUi(this);

    mPngquantProc->setInputChannelMode(QProcess::ManagedInputChannel);
    mPngquantProc->setProgram(QLatin1String("pngquant"));
    mPngquantProc->setArguments(QStringList() << QLatin1String("--strip") << QLatin1String("7") << QLatin1String("-"));

    mUi->faceConfig->setTitle(i18n("Face"));
    mUi->xFaceConfig->setTitle(i18n("X-Face"));
    mUi->faceConfig->setInfo(i18n("More information under <a href=\"https://quimby.gnus.org/circus/face/\">https://quimby.gnus.org/circus/face/</a>."));
    mUi->xFaceConfig->setInfo(i18n("Examples are available at <a href=\"https://ace.home.xs4all.nl/X-Faces/\">https://ace.home.xs4all.nl/X-Faces/</a>."));

    connect(mUi->enableComboBox, &QComboBox::currentIndexChanged, this, &XFaceConfigurator::modeChanged);
    connect(mUi->faceConfig, &EncodedImagePicker::imageSelected, this, &XFaceConfigurator::compressFace);
    connect(mUi->xFaceConfig, &EncodedImagePicker::imageSelected, this, &XFaceConfigurator::compressXFace);
    connect(mUi->faceConfig, &EncodedImagePicker::sourceChanged, this, &XFaceConfigurator::updateFace);
    connect(mUi->xFaceConfig, &EncodedImagePicker::sourceChanged, this, &XFaceConfigurator::updateXFace);
    connect(mPngquantProc, &QProcess::finished, this, &XFaceConfigurator::pngquantFinished);

    // set initial state
    modeChanged(mUi->enableComboBox->currentIndex());
}

XFaceConfigurator::~XFaceConfigurator() = default;

bool XFaceConfigurator::isXFaceEnabled() const
{
    return mUi->enableComboBox->currentIndex() & SendXFace;
}

void XFaceConfigurator::setXFaceEnabled(bool enable)
{
    const int currentIndex = mUi->enableComboBox->currentIndex();

    if (enable) {
        mUi->enableComboBox->setCurrentIndex(currentIndex | SendXFace);
    } else {
        mUi->enableComboBox->setCurrentIndex(currentIndex & ~SendXFace);
    }
}

bool XFaceConfigurator::isFaceEnabled() const
{
    return mUi->enableComboBox->currentIndex() & SendFace;
}

void XFaceConfigurator::setFaceEnabled(bool enable)
{
    const int currentIndex = mUi->enableComboBox->currentIndex();

    if (enable) {
        mUi->enableComboBox->setCurrentIndex(currentIndex | SendFace);
    } else {
        mUi->enableComboBox->setCurrentIndex(currentIndex & ~SendFace);
    }
}

QString XFaceConfigurator::xface() const
{
    QString str = mUi->xFaceConfig->source().trimmed();
    str.remove(QStringLiteral("x-face:"), Qt::CaseInsensitive);
    str = str.trimmed();

    return str;
}

void XFaceConfigurator::setXFace(const QString &text)
{
    mUi->xFaceConfig->setSource(text);
}

QString XFaceConfigurator::face() const
{
    QString str = mUi->faceConfig->source().trimmed();
    str.remove(QStringLiteral("face:"), Qt::CaseInsensitive);
    str = str.trimmed();

    return str;
}

void XFaceConfigurator::setFace(const QString &text)
{
    mUi->faceConfig->setSource(text);
}

void XFaceConfigurator::modeChanged(int index)
{
    mUi->faceConfig->setEnabled(index & SendFace);
    mUi->xFaceConfig->setEnabled(index & SendXFace);

    switch (index) {
    case DontSend:
        mUi->modeInfo->setText(i18n("No image will be sent."));
        break;
    case SendFace:
        mUi->modeInfo->setText(i18n("KMail will send a colored image through the Face header."));
        break;
    case SendXFace:
        mUi->modeInfo->setText(i18n("KMail will send a black-and-white image through the X-Face header."));
        break;
    case SendBoth:
        mUi->modeInfo->setText(i18n("KMail will send both a colored and a black-and-white image."));
        break;
    }
}

void XFaceConfigurator::updateFace()
{
    const QString str = face();
    const QByteArray facearray = QByteArray::fromBase64(str.toUtf8());
    QImage faceimage;

    faceimage.loadFromData(facearray, "png");
    mUi->faceConfig->setImage(faceimage);
}

void XFaceConfigurator::updateXFace()
{
    const QString str = xface();

    if (str.isEmpty()) {
        mUi->xFaceConfig->setImage(QImage());
    } else {
        KXFace xf;
        mUi->xFaceConfig->setImage(xf.toImage(str));
    }
}

void XFaceConfigurator::compressFace(const QImage &image)
{
    if (!pngquant(image)) {
        crunch(image);
    }
}

void XFaceConfigurator::compressFaceDone(const QByteArray &data, bool fromPngquant)
{
    if (data.isNull()) {
        if (fromPngquant) {
            KMessageBox::error(this, i18n("Failed to reduce image size to fit in header."));
        } else {
            KMessageBox::error(this, i18n("Failed to reduce image size to fit in header. Install pngquant to obtain better compression results."));
        }
        return;
    }

    mUi->faceConfig->setSource(QString::fromUtf8(data.toBase64()));

    if (!fromPngquant) {
        KMessageBox::information(this, i18n("Install pngquant to obtain better image quality."));
    }
}

void XFaceConfigurator::compressXFace(const QImage &image)
{
    KXFace xf;
    const QString xFaceString = xf.fromImage(image);
    mUi->xFaceConfig->setSource(xFaceString);
}

// The builtin image compressor. It's pretty bad and pngquant is preferred when
// available.
void XFaceConfigurator::crunch(const QImage &image)
{
    QImage output;
    QByteArray ba;
    int crunchLevel = 0;
    int maxCrunchLevel = 6 * 5; // 6 formats, 5 sizes

    const QImage::Format formats[6] = {
        QImage::Format_RGB32,
        QImage::Format_RGB888,
        QImage::Format_RGB16,
        QImage::Format_RGB666,
        QImage::Format_RGB555,
        QImage::Format_RGB444,
    };

    int sizes[5] = {48, 24, 12, 6, 3};

    while (true) {
        const QImage::Format targetFormat = formats[crunchLevel % 6];
        int targetSize = sizes[crunchLevel / 6];
        output = image;

        if (targetSize != 48) {
            output = output.scaled(targetSize, targetSize);
        }

        output = output.scaled(48, 48);
        ba.clear();
        QBuffer buffer(&ba);

        buffer.open(QIODevice::WriteOnly);
        output.convertTo(targetFormat);
        output.save(&buffer, "PNG", 0);

        if (ba.size() <= FACE_MAX_SIZE) {
            compressFaceDone(ba, false);
            break;
        } else if (crunchLevel < maxCrunchLevel - 1) {
            crunchLevel += 1;
        } else {
            compressFaceDone(QByteArray(), false);
            break;
        }
    }
}

bool XFaceConfigurator::pngquant(const QImage &image)
{
    const QImage small = image.scaled(48, 48);

    mPngquantProc->terminate();
    mPngquantProc->waitForFinished();

    mPngquantProc->start();

    if (mPngquantProc->waitForStarted()) {
        small.save(mPngquantProc, "PNG");
        mPngquantProc->closeWriteChannel();
        return true;
    } else {
        return false;
    }
}

void XFaceConfigurator::pngquantFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        const QByteArray output = mPngquantProc->readAllStandardOutput();
        compressFaceDone(output, true);
    } else {
        const QByteArray errOut = mPngquantProc->readAllStandardError();
        const QString str = QString::fromLocal8Bit(errOut);

        KMessageBox::error(this, i18n("pngquant exited with code %1: %2", exitCode, str));

        compressFaceDone(QByteArray(), true);
    }
}
