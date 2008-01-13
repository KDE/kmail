/*
 *  File : snippetitem.h
 *
 *  Author: Robert Gruber <rgruber@users.sourceforge.net>
 *
 *  Copyright: See COPYING file that comes with this distribution
 */

#ifndef SNIPPETITEM_H
#define SNIPPETITEM_H

#include <qlistview.h>
#include <QTreeWidgetItem>
#include <klocale.h>

class QString;

class SnippetGroup;

/**
This class represents one CodeSnippet-Item in the listview.
It also holds the needed data for one snippet.
@author Robert Gruber
*/
class SnippetItem : public QTreeWidgetItem {
friend class SnippetGroup;
public:
    SnippetItem(QTreeWidgetItem * parent, const QString &name, const QString &text);

    ~SnippetItem();
    QString getName();
    QString getText();
    int getParent() { return iParent; }
    void resetParent();
    void setText(const QString &text);
    void setName(const QString &name);
    static SnippetItem *findItemByName(const QString &name, const QList<SnippetItem * > &list);
    static SnippetGroup *findGroupById(int id, const QList<SnippetItem * > &list);

    //reimp
    QVariant data(int column, int role);

private:
  SnippetItem( QTreeWidget *parent, const QString &name, const QString &text );
  QString strName;
  QString strText;
  int iParent;
};

/**
This class represents one group in the listview.
It is derived from SnippetItem in order to allow storing 
it in the main QPtrList<SnippetItem>.
@author Robert Gruber
*/
class SnippetGroup : public SnippetItem {
public:
    SnippetGroup( QTreeWidget *parent, const QString &name, int id );
    ~SnippetGroup();

    int getId() { return iId; }
    static int getMaxId() { return iMaxId; }

    void setId(int id);

private:
    static int iMaxId;
    int iId;
};

#endif
