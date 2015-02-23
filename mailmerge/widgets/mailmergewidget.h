/*
  Copyright (c) 2014-2015 Montel Laurent <montel@kde.org>

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

#ifndef MAILMERGEWIDGET_H
#define MAILMERGEWIDGET_H

#include <QWidget>

class KComboBox;
class QStackedWidget;

class KUrlRequester;
namespace MailMerge {
class CsvWidget;
class AttachmentListWidget;
class AddressBookWidget;
class MailMergeWidget : public QWidget
{
    Q_OBJECT
public:
    enum SourceType {
        AddressBook = 0,
        CSV = 1
    };

    explicit MailMergeWidget(QWidget *parent = 0);
    ~MailMergeWidget();

Q_SIGNALS:
    void sourceModeChanged(MailMerge::MailMergeWidget::SourceType);

private slots:
    void slotSourceChanged(int index);

private:
    KComboBox *mSource;
    QStackedWidget *mStackedWidget;
    AttachmentListWidget *mAttachment;
    MailMerge::CsvWidget *mCsvWidget;
    MailMerge::AddressBookWidget *mAddressbookWidget;
};
}
#endif // MAILMERGEWIDGET_H
