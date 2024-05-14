/*
  SPDX-FileCopyrightText: 2015-2024 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: LGPL-2.0-or-later

*/

#include "potentialphishingemailwarning.h"
using namespace Qt::Literals::StringLiterals;

#include "potentialphishingdetaildialog.h"
#include <KLocalizedString>
#include <QAction>

PotentialPhishingEmailWarning::PotentialPhishingEmailWarning(QWidget *parent)
    : KMessageWidget(parent)
{
    setVisible(false);
    setCloseButtonVisible(true);
    setMessageType(Warning);
    setWordWrap(true);
    setPosition(KMessageWidget::Header);

    setText(i18n("Found potential phishing email addresses <a href=\"phishingdetails\">(details…)</a>"));

    connect(this, &KMessageWidget::linkActivated, this, &PotentialPhishingEmailWarning::slotShowDetails);
    auto action = new QAction(i18nc("@action", "Send Now"), this);
    action->setObjectName("sendnow"_L1);
    connect(action, &QAction::triggered, this, &PotentialPhishingEmailWarning::sendNow);
    addAction(action);
}

PotentialPhishingEmailWarning::~PotentialPhishingEmailWarning() = default;

void PotentialPhishingEmailWarning::slotShowDetails(const QString &link)
{
    if (link == "phishingdetails"_L1) {
        PotentialPhishingDetailDialog dlg(this);
        dlg.fillList(mPotentialPhishingEmails);
        dlg.exec();
    }
}

void PotentialPhishingEmailWarning::setPotentialPhisingEmail(const QStringList &lst)
{
    mPotentialPhishingEmails = lst;
    if (!mPotentialPhishingEmails.isEmpty()) {
        animatedShow();
    }
}

#include "moc_potentialphishingemailwarning.cpp"
