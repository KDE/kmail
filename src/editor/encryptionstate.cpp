/*
  SPDX-FileCopyrightText: 2022 Sandro Knauß <sknauss@kde.org>
  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "encryptionstate.h"

EncryptionState::EncryptionState()
{
    connect(this, &EncryptionState::acceptedSolutionChanged, this, &EncryptionState::updateEncrypt);
    connect(this, &EncryptionState::overrideChanged, this, &EncryptionState::updateEncrypt);
    connect(this, &EncryptionState::possibleEncryptChanged, this, &EncryptionState::updateEncrypt);
    connect(this, &EncryptionState::autoEncryptChanged, this, &EncryptionState::updateEncrypt);
}

bool EncryptionState::override() const
{
    return m_override;
}

void EncryptionState::setOverride(bool setByUser)
{
    if (!m_hasOverride) {
        m_hasOverride = true;
        Q_EMIT hasOverrideChanged(true);
        // Before the override setting was undefined, we should trigger a signal with correct state
        m_override = setByUser;
        Q_EMIT overrideChanged(setByUser);
        return;
    }

    if (m_override == setByUser) {
        return;
    }

    m_override = setByUser;
    Q_EMIT overrideChanged(m_override);
}

void EncryptionState::toggleOverride()
{
    setOverride(!encrypt());
}

void EncryptionState::unsetOverride()
{
    if (!m_hasOverride) {
        return;
    }
    m_hasOverride = false;
    Q_EMIT hasOverrideChanged(false);
    Q_EMIT overrideChanged(false);
}

bool EncryptionState::hasOverride() const
{
    return m_hasOverride;
}

bool EncryptionState::acceptedSolution() const
{
    return m_acceptedSolution;
}

void EncryptionState::setAcceptedSolution(bool acceptedSolution)
{
    if (m_acceptedSolution == acceptedSolution) {
        return;
    }

    m_acceptedSolution = acceptedSolution;
    Q_EMIT acceptedSolutionChanged(m_acceptedSolution);
}

bool EncryptionState::possibleEncrypt() const
{
    return m_possibleEncrypt;
}

void EncryptionState::setPossibleEncrypt(bool possibleEncrypt)
{
    if (m_possibleEncrypt == possibleEncrypt) {
        return;
    }

    m_possibleEncrypt = possibleEncrypt;
    Q_EMIT possibleEncryptChanged(m_possibleEncrypt);
}

bool EncryptionState::autoEncrypt() const
{
    return m_autoEncrypt;
}

void EncryptionState::setAutoEncrypt(bool autoEncrypt)
{
    if (m_autoEncrypt == autoEncrypt) {
        return;
    }

    m_autoEncrypt = autoEncrypt;
    Q_EMIT autoEncryptChanged(m_autoEncrypt);
}

void EncryptionState::setEncrypt(bool encrypt)
{
    if (m_encrypt == encrypt) {
        return;
    }

    m_encrypt = encrypt;
    Q_EMIT encryptChanged(m_encrypt);
}

void EncryptionState::updateEncrypt()
{
    if (m_hasOverride) {
        setEncrypt(m_override);
        return;
    }
    if (!m_possibleEncrypt) {
        setEncrypt(false);
        return;
    }

    if (!m_autoEncrypt) {
        setEncrypt(false);
        return;
    }

    setEncrypt(m_acceptedSolution);
}

bool EncryptionState::encrypt() const
{
    return m_encrypt;
}

#include "moc_encryptionstate.cpp"
