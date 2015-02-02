/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/


#include "potentialphishingemailwarning.h"
#include "potentialphishingdetaildialog.h"
#include <KAction>
#include <KIcon>
#include <KLocalizedString>
#include <QPointer>

PotentialPhishingEmailWarning::PotentialPhishingEmailWarning(QWidget *parent)
    : KMessageWidget(parent)
{
    setVisible(false);
    setCloseButtonVisible(true);
    setMessageType(Warning);
    setWordWrap(true);

    //Add i18n in kf5
    setText(QLatin1String("Some address mail seems a potential phishing email <a href=\"phishingdetails\">(Details...)</a>"));

    connect(this, SIGNAL(linkActivated(QString)), SLOT(slotShowDetails(QString)));
    //Add i18n in kf5
    QAction *action = new QAction(QLatin1String( "Send Now" ), this );
    action->setObjectName(QLatin1String("sendnow"));
    connect( action, SIGNAL(triggered(bool)), SIGNAL(sendNow()) );
    addAction( action );
}

PotentialPhishingEmailWarning::~PotentialPhishingEmailWarning()
{

}

void PotentialPhishingEmailWarning::slotShowDetails(const QString &link)
{
    if (link == QLatin1String("phishingdetails")) {
        QPointer<PotentialPhishingDetailDialog> dlg = new PotentialPhishingDetailDialog(this);
        dlg->fillList(mPotentialPhishingEmails);
        dlg->exec();
        delete dlg;
    }
}

void PotentialPhishingEmailWarning::setPotentialPhisingEmail(const QStringList &lst)
{
     mPotentialPhishingEmails = lst;
     if (!mPotentialPhishingEmails.isEmpty()) {
         animatedShow();
     }
}
