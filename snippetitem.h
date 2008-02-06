/***************************************************************************
 *   snippet feature from kdevelop/plugins/snippet/                        *
 *                                                                         * 
 *   Copyright (C) 2007 by Robert Gruber                                   *
 *   rgruber@users.sourceforge.net                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SNIPPETITEM_H
#define SNIPPETITEM_H

#include <klistview.h>
#include <klocale.h>

#include <qobject.h>

class QString;
class KAction;
class SnippetGroup;


/**
This class represents one CodeSnippet-Item in the listview.
It also holds the needed data for one snippet.
@author Robert Gruber
*/
class SnippetItem : public QObject, public QListViewItem {
friend class SnippetGroup;

    Q_OBJECT
public:
    SnippetItem(QListViewItem * parent, QString name, QString text);

    ~SnippetItem();
    QString getName();
    QString getText();
    using QListViewItem::parent;
    int getParent() { return iParent; }
    void resetParent();
    void setText(QString text);
    void setName(QString name);
    void setAction( KAction* );
    KAction* getAction();
    static SnippetItem * findItemByName(QString name, QPtrList<SnippetItem> &list);
    static SnippetGroup * findGroupById(int id, QPtrList<SnippetItem> &list);
signals:
    void execute( QListViewItem * );
public slots:
    void slotExecute();
    
private:
  SnippetItem(QListView * parent, QString name, QString text);
  QString strName;
  QString strText;
  int iParent;
  KAction *action;
};

/**
This class represents one group in the listview.
It is derived from SnippetItem in order to allow storing 
it in the main QPtrList<SnippetItem>.
@author Robert Gruber
*/
class SnippetGroup : public SnippetItem {
public:
    SnippetGroup(QListView * parent, QString name, int id);
    ~SnippetGroup();

    int getId() { return iId; }
    static int getMaxId() { return iMaxId; }
    
    void setId(int id);

private:
    static int iMaxId;
    int iId;
};

#endif
