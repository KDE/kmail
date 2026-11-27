/*  -*- c++ -*-
    SPDX-FileCopyrightText: 2008 Thomas McGuire <Thomas.McGuire@gmx.net>
    SPDX-FileCopyrightText: 2008 Edwin Schepers <yez@familieschepers.nl>
    SPDX-FileCopyrightText: 2008 Tom Albers <tomalbers@kde.nl>
    SPDX-FileCopyrightText: 2004 Marc Mutz <mutz@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <KIdentityManagementCore/Signature> // for Signature::Type
#include <QWidget>
#include <memory>

namespace KIdentityManagementCore
{
class Identity;
}

using KIdentityManagementCore::Signature;

namespace KMail
{
class SignatureConfiguratorPrivate;
/*!
 * \class KMail::SignatureConfigurator
 *
 * This widget gives an interface so users can edit their signature.
 * You can set a signature via setSignature(), let the user edit the
 * signature and when done, read the signature back.
 */
class SignatureConfigurator : public QWidget
{
    Q_OBJECT
public:
    /*!
     * Constructs a signature configurator with the given \a parent.
     */
    explicit SignatureConfigurator(QWidget *parent = nullptr);

    /*!
     * destructor
     */
    ~SignatureConfigurator() override;

    /*!
     * Enum for the different viewmodes.
     *
     * \value ShowCode Show the signature source code
     * \value ShowHtml Show the rendered HTML signature
     */
    enum ViewMode {
        ShowCode,
        ShowHtml
    };

    /*!
     * Returns \c true if the user wants a signature.
     */
    [[nodiscard]] bool isSignatureEnabled() const;

    /*!
     * Use this to activate the signature: \a enable is \c true to enable it.
     */
    void setSignatureEnabled(bool enable);

    /*!
     * This returns the type of the signature,
     * so that can be Disabled, Inline, fromFile, etc.
     */
    [[nodiscard]] Signature::Type signatureType() const;

    /*!
     * Set the signature type to \a type.
     */
    void setSignatureType(Signature::Type type);

    /*!
     * Make \a text the text for the signature.
     */
    void setInlineText(const QString &text);

    /*!
     * Returns the file url which the user wants
     * to use as a signature.
     */
    [[nodiscard]] QString filePath() const;

    /*!
     * Set \a url for the file url part of the
     * widget.
     */
    void setFileURL(const QString &url);

    /*!
     * Returns the url of the command which the
     * users wants to use as signature.
     */
    [[nodiscard]] QString commandPath() const;

    /*!
     * Sets \a url as the command to execute.
     */
    void setCommandURL(const QString &url);

    /*!
       Convenience method.
       Returns a Signature object representing the state of the widgets.
     **/
    [[nodiscard]] Signature signature() const;

    /*!
       Convenience method. Sets the widgets according to \a sig
       \a sig the signature to configure
    **/
    void setSignature(const Signature &sig);

    /*!
     * Sets the directory where the images used in the HTML signature will be stored.
     * Needs to be called before calling setSignature(), as each signature should use
     * a different location.
     * The directory needs to exist, it will not be created.
     * \a path the image location to set
     * \since 4.4
     * @sa Signature::setImageLocation
     */
    void setImageLocation(const QString &path);

    /*!
     * Sets the image location to the image location of a given identity, which is
     * emailidentities/<identity-id>/.
     *
     * \a identity The identity whose unique ID will be used to determine the image
     *                 location.
     * \since 4.4
     */
    void setImageLocation(const KIdentityManagementCore::Identity &identity);

private:
    void slotUrlChanged();
    void slotEdit();
    void slotSetHtml();

    friend class SignatureConfiguratorPrivate;
    std::unique_ptr<SignatureConfiguratorPrivate> const d;
};
}
