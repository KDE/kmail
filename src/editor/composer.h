/*
  This file is part of KMail, the KDE mail client.
  SPDX-FileCopyrightText: 1997 Markus Wuebben <markus.wuebben@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "kmail_export.h"
#include "secondarywindow.h"

#include <AkonadiCore/collection.h>
#include <QUrl>
#include <kmime/kmime_message.h>

namespace KMime
{
class Content;
}

namespace KMail
{
class KMAIL_EXPORT Composer : public KMail::SecondaryWindow
{
    Q_OBJECT
protected:
    Composer(const QString &name = QString())
        : KMail::SecondaryWindow(name)
    {
    }

public:
    enum TemplateContext {
        New,
        Reply,
        ReplyToAll,
        Forward,
        NoTemplate,
    };
    enum VisibleHeaderFlag {
        HDR_FROM = 0x01,
        HDR_REPLY_TO = 0x02,
        HDR_SUBJECT = 0x20,
        HDR_IDENTITY = 0x100,
        HDR_TRANSPORT = 0x200,
        HDR_FCC = 0x400,
        HDR_DICTIONARY = 0x800,
        HDR_ALL = 0xfff,
    };
    using VisibleHeaderFlags = QFlags<VisibleHeaderFlag>;

    struct AttachmentInfo {
        QString comment;
        QUrl url;
    };

public: // mailserviceimpl
    /**
     * From MailComposerIface
     */
    virtual void send(int how) = 0;
    virtual void addAttachmentsAndSend(const QList<QUrl> &urls, const QString &comment, int how) = 0;
    virtual void addAttachment(const QVector<AttachmentInfo> &url, bool showWarning) = 0;
    virtual void
    addAttachment(const QString &name, KMime::Headers::contentEncoding cte, const QString &charset, const QByteArray &data, const QByteArray &mimeType) = 0;

public: // kmcommand
    virtual Q_REQUIRED_RESULT QString dbusObjectPath() const = 0;

public: // kmkernel, kmcommands, callback
    /**
     * Set the message the composer shall work with. This discards
     * previous messages without calling applyChanges() on them before.
     */
    virtual void setMessage(const KMime::Message::Ptr &newMsg,
                            bool lastSignState = false,
                            bool lastEncryptState = false,
                            bool mayAutoSign = true,
                            bool allowDecryption = false,
                            bool isModified = false) = 0;
    virtual void setCurrentTransport(int transportId) = 0;

    virtual void setFcc(const QString &idString) = 0;
    /**
     * Returns @c true while the message composing is in progress.
     */
    virtual bool isComposing() const = 0;

    virtual void setAutoSaveFileName(const QString &fileName) = 0;
    virtual void setCollectionForNewMessage(const Akonadi::Collection &folder) = 0;

    virtual void addExtraCustomHeaders(const QMap<QByteArray, QString> &header) = 0;

    virtual void showAndActivateComposer() = 0;

public: // kmcommand
    /**
     * If this folder is set, the original message is inserted back after
     * canceling
     */
    virtual void setFolder(const Akonadi::Collection &) = 0;

    /**
     * Sets the focus to the edit-widget and the cursor below the
     * "On ... you wrote" line when hasMessage is true.
     * Make sure you call this _after_ setMsg().
     */
    virtual void setFocusToEditor() = 0;

    /**
     * Sets the focus to the subject line edit. For use when creating a
     * message to a known recipient.
     */
    virtual void setFocusToSubject() = 0;

public: // callback
    /** Disabled signing and encryption completely for this composer window. */
    virtual void setSigningAndEncryptionDisabled(bool v) = 0;

public Q_SLOTS: // kmkernel, callback
    virtual void slotSendNow() = 0;
    virtual void setModified(bool modified) = 0;
public Q_SLOTS: // kmkernel
    virtual void autoSaveMessage(bool force = false) = 0;

public: // kmkernel
    virtual void disableWordWrap() = 0;

    virtual void forceDisableHtml() = 0;

    virtual void disableForgottenAttachmentsCheck() = 0;

public: // kmcommand
    /**
     * Add an attachment to the list.
     */
    virtual void addAttach(KMime::Content *msgPart) = 0;
};

KMAIL_EXPORT Composer *makeComposer(const KMime::Message::Ptr &msg = KMime::Message::Ptr(),
                                    bool lastSignState = false,
                                    bool lastEncryptState = false,
                                    Composer::TemplateContext context = Composer::NoTemplate,
                                    uint identity = 0,
                                    const QString &textSelection = QString(),
                                    const QString &customTemplate = QString());
}

