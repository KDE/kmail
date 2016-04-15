/*
  Copyright (c) 2015-2016 Montel Laurent <montel@kde.org>

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
#ifndef SAVEASFILEJOB_H
#define SAVEASFILEJOB_H

#include <QObject>
class QTextDocument;
class SaveAsFileJob : public QObject
{
    Q_OBJECT
public:
    explicit SaveAsFileJob(QObject *parent = Q_NULLPTR);
    ~SaveAsFileJob();
    void start();

    void setHtmlMode(bool htmlMode);

    void setTextDocument(QTextDocument *textDocument);

    void setParentWidget(QWidget *parentWidget);

private:
    bool mHtmlMode;
    QTextDocument *mTextDocument;
    QWidget *mParentWidget;
};

#endif // SAVEASFILEJOB_H
