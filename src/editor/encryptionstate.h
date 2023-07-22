/*
  SPDX-FileCopyrightText: 2022 Sandro Knauß <sknauss@kde.org>
  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once
#include "kmail_private_export.h"
#include <QObject>

/**
 * @todo write docs
 */
class KMAILTESTS_TESTS_EXPORT EncryptionState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool override READ override WRITE setOverride NOTIFY overrideChanged)
    Q_PROPERTY(bool possibleEncrypt READ possibleEncrypt WRITE setPossibleEncrypt NOTIFY possibleEncryptChanged)
    Q_PROPERTY(bool autoEncrypt READ autoEncrypt WRITE setAutoEncrypt NOTIFY autoEncryptChanged)
    Q_PROPERTY(bool acceptedSolution READ acceptedSolution WRITE setAcceptedSolution NOTIFY acceptedSolutionChanged)
    Q_PROPERTY(bool encrypt READ encrypt NOTIFY encryptChanged)

public:
    /**
     * Default constructor
     */
    EncryptionState();

    /**
     * @return the user set the encryption state no matter what
     */
    bool override() const;

    /**
     * @return true when set an override
     */
    bool hasOverride() const;

    /**
     * @return we have encryption keys for the user so in principal it is possible to encrypt
     */
    bool possibleEncrypt() const;

    /**
     * @return the user wants auto encryption
     */
    bool autoEncrypt() const;

    /**
     * @return we found a set of keys to encrypt to everyone
     */
    bool acceptedSolution() const;

    /**
     * @return the encrypt
     */
    bool encrypt() const;

public Q_SLOTS:
    /**
     * Sets the override.
     *
     * @param override the new override
     */
    void setOverride(bool override);

    /**
     * Delete the override.
     */
    void unsetOverride();

    /**
     * Toggles the override
     */
    void toggleOverride();

    /**
     * Sets the acceptedSolution.
     *
     * @param acceptedSolution the new acceptedSolution
     */
    void setAcceptedSolution(bool acceptedSolution);

    /**
     * Sets the possibleEncrypt.
     *
     * @param possibleEncrypt the new possibleEncrypt
     */
    void setPossibleEncrypt(bool possibleEncrypt);

    /**
     * Sets the autoEncrypt.
     *
     * @param autoEncrypt the new autoEncrypt
     */
    void setAutoEncrypt(bool autoEncrypt);

Q_SIGNALS:
    void overrideChanged(bool override);
    void hasOverrideChanged(bool hasOverride);

    void acceptedSolutionChanged(bool acceptedSolution);

    void possibleEncryptChanged(bool possibleEncrypt);

    void autoEncryptChanged(bool autoEncrypt);

    void encryptChanged(bool encrypt);

private:
    void setEncrypt(bool encrypt);
    void updateEncrypt();

private:
    bool m_override = false;
    bool m_hasOverride = false;
    bool m_acceptedSolution = false;
    bool m_possibleEncrypt = false;
    bool m_autoEncrypt = false;
    bool m_encrypt = false;
};
