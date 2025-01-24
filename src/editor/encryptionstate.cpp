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
    return mOverride;
}

void EncryptionState::setOverride(bool setByUser)
{
    if (!mHasOverride) {
        mHasOverride = true;
        Q_EMIT hasOverrideChanged(true);
        // Before the override setting was undefined, we should trigger a signal with correct state
        mOverride = setByUser;
        Q_EMIT overrideChanged(setByUser);
        return;
    }

    if (mOverride == setByUser) {
        return;
    }

    mOverride = setByUser;
    Q_EMIT overrideChanged(mOverride);
}

void EncryptionState::toggleOverride()
{
    setOverride(!encrypt());
}

void EncryptionState::unsetOverride()
{
    if (!mHasOverride) {
        return;
    }
    mHasOverride = false;
    Q_EMIT hasOverrideChanged(false);
    Q_EMIT overrideChanged(false);
}

bool EncryptionState::hasOverride() const
{
    return mHasOverride;
}

bool EncryptionState::acceptedSolution() const
{
    return mAcceptedSolution;
}

void EncryptionState::setAcceptedSolution(bool acceptedSolution)
{
    if (mAcceptedSolution == acceptedSolution) {
        return;
    }

    mAcceptedSolution = acceptedSolution;
    Q_EMIT acceptedSolutionChanged(mAcceptedSolution);
}

bool EncryptionState::possibleEncrypt() const
{
    return mPossibleEncrypt;
}

void EncryptionState::setPossibleEncrypt(bool possibleEncrypt)
{
    if (mPossibleEncrypt == possibleEncrypt) {
        return;
    }

    mPossibleEncrypt = possibleEncrypt;
    Q_EMIT possibleEncryptChanged(mPossibleEncrypt);
}

bool EncryptionState::autoEncrypt() const
{
    return mAutoEncrypt;
}

void EncryptionState::setAutoEncrypt(bool autoEncrypt)
{
    if (mAutoEncrypt == autoEncrypt) {
        return;
    }

    mAutoEncrypt = autoEncrypt;
    Q_EMIT autoEncryptChanged(mAutoEncrypt);
}

void EncryptionState::setEncrypt(bool encrypt)
{
    if (mEncrypt == encrypt) {
        return;
    }

    mEncrypt = encrypt;
    Q_EMIT encryptChanged(mEncrypt);
}

void EncryptionState::updateEncrypt()
{
    if (mHasOverride) {
        setEncrypt(mOverride);
        return;
    }
    if (!mPossibleEncrypt) {
        setEncrypt(false);
        return;
    }

    if (!mAutoEncrypt) {
        setEncrypt(false);
        return;
    }

    setEncrypt(mAcceptedSolution);
}

bool EncryptionState::encrypt() const
{
    return mEncrypt;
}

#include "moc_encryptionstate.cpp"
