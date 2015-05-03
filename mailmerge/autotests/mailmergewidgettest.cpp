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

#include "mailmergewidgettest.h"
#include "../widgets/mailmergewidget.h"
#include "pimcommon/widgets/simplestringlisteditor.h"
#include <qtest.h>
#include <KComboBox>
#include <QStackedWidget>
#include <QSignalSpy>
using namespace MailMerge;
Q_DECLARE_METATYPE(MailMerge::MailMergeWidget::SourceType)
MailMergeWidgetTest::MailMergeWidgetTest()
{
    qRegisterMetaType<MailMergeWidget::SourceType>();
}

void MailMergeWidgetTest::shouldHaveDefaultValueOnCreation()
{
    MailMergeWidget mailmerge;
    KComboBox *source = mailmerge.findChild<KComboBox *>(QStringLiteral("source"));
    QVERIFY(source);
    QCOMPARE(source->currentIndex(), 0);

    QStackedWidget *stackedwidget = mailmerge.findChild<QStackedWidget *>(QStringLiteral("stackedwidget"));
    QVERIFY(stackedwidget);
    QCOMPARE(stackedwidget->count(), 2);
    QCOMPARE(stackedwidget->currentIndex(), 0);

    for (int i = 0; i < stackedwidget->count(); ++i) {
        const QString objectName = stackedwidget->widget(i)->objectName();
        bool hasName = (objectName == QLatin1String("addressbookwidget") ||
                        objectName == QLatin1String("csvwidget"));
        QVERIFY(hasName);
    }
    PimCommon::SimpleStringListEditor *listEditor = mailmerge.findChild<PimCommon::SimpleStringListEditor *>(QStringLiteral("attachment-list"));
    QVERIFY(listEditor);
    QCOMPARE(listEditor->stringList().count(), 0);
}

void MailMergeWidgetTest::shouldEmitSourceModeChanged()
{
    MailMergeWidget mailmerge;
    KComboBox *source = mailmerge.findChild<KComboBox *>(QStringLiteral("source"));
    QCOMPARE(source->currentIndex(), 0);
    QSignalSpy spy(&mailmerge, SIGNAL(sourceModeChanged(MailMerge::MailMergeWidget::SourceType)));
    source->setCurrentIndex(1);
    QCOMPARE(spy.count(), 1);
}

void MailMergeWidgetTest::shouldDontEmitSourceModeChangedWhenIndexIsInvalid()
{
    MailMergeWidget mailmerge;
    KComboBox *source = mailmerge.findChild<KComboBox *>(QStringLiteral("source"));
    QCOMPARE(source->currentIndex(), 0);
    QSignalSpy spy(&mailmerge, SIGNAL(sourceModeChanged(MailMerge::MailMergeWidget::SourceType)));
    source->setCurrentIndex(-1);
    QCOMPARE(spy.count(), 0);
}

void MailMergeWidgetTest::shouldChangeStackedWidgetIndexWhenChangeComboboxIndex()
{
    MailMergeWidget mailmerge;
    KComboBox *source = mailmerge.findChild<KComboBox *>(QStringLiteral("source"));
    QCOMPARE(source->currentIndex(), 0);

    QStackedWidget *stackedwidget = mailmerge.findChild<QStackedWidget *>(QStringLiteral("stackedwidget"));
    QCOMPARE(stackedwidget->currentIndex(), 0);
    source->setCurrentIndex(0);
    QCOMPARE(stackedwidget->currentIndex(), 0);
    source->setCurrentIndex(1);
    QCOMPARE(stackedwidget->currentIndex(), 1);
}

QTEST_MAIN(MailMergeWidgetTest)
