/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "selectattachmentdialog.h"
#include <KLocalizedString>
#include <kurlrequester.h>
#include <qboxlayout.h>
#include <qlabel.h>

using namespace MailMerge;

SelectAttachmentDialog::SelectAttachmentDialog(QWidget *parent)
    : KDialog(parent)
{
    setCaption(i18n("Attachment"));
    setButtons(Ok|Cancel);

    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout;
    mainWidget->setLayout(vbox);
    QLabel *lab = new QLabel(i18n("Select attachment:"));
    lab->setObjectName(QLatin1String("selectattachment_label"));
    vbox->addWidget(lab);
    mUrlRequester = new KUrlRequester;
    mUrlRequester->setObjectName(QLatin1String("urlrequester"));
    mUrlRequester->setMode(KFile::LocalOnly|KFile::ExistingOnly);
    vbox->addWidget(mUrlRequester);
    setMainWidget(mainWidget);
}

SelectAttachmentDialog::~SelectAttachmentDialog()
{

}

void SelectAttachmentDialog::setAttachmentPath(const QString &path)
{
    mUrlRequester->setUrl(path);
}

QString SelectAttachmentDialog::attachmentPath() const
{
    return mUrlRequester->url().path();
}

