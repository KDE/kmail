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

#include "csvwidget.h"

#include <KUrlRequester>
#include <QLabel>
#include <QVBoxLayout>
#include <KLocalizedString>

using namespace MailMerge;

CsvWidget::CsvWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *csvWidgetLayout = new QVBoxLayout;
    csvWidgetLayout->setMargin(0);
    setLayout(csvWidgetLayout);

    QLabel *lab = new QLabel(i18n("Path:"));
    lab->setObjectName(QLatin1String("label"));
    csvWidgetLayout->addWidget(lab);
    mCvsUrlRequester = new KUrlRequester;
    mCvsUrlRequester->setObjectName(QLatin1String("cvsurlrequester"));
    csvWidgetLayout->addWidget(mCvsUrlRequester);
}

CsvWidget::~CsvWidget()
{

}

void CsvWidget::setPath(const KUrl &path)
{
    mCvsUrlRequester->setUrl(path);
}

KUrl CsvWidget::path() const
{
    return mCvsUrlRequester->url();
}

