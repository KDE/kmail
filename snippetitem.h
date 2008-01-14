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

#include <qobject.h>

class QString;
class KAction;
class SnippetGroup;


/**
This class represents one CodeSnippet-Item in the listview.
It also holds the needed data for one snippet.
@author Robert Gruber
*/
class SnippetItem : public QObject, public QTreeWidgetItem {
friend class SnippetGroup;

    Q_OBJECT
public:
    SnippetItem(QTreeWidgetItem * parent, const QString &name, const QString &text);

    ~SnippetItem();
    QString getName() const;
    QString getText() const;
    using QTreeWidgetItem::parent;
    int getParent() const { return iParent; }
    void resetParent();
    void setText(const QString &text);
    void setName(const QString &name);
    static SnippetItem *findItemByName(const QString &name, const QList<SnippetItem * > &list);
    static SnippetGroup *findGroupById(int id, const QList<SnippetItem * > &list);
    void setAction( KAction* );
    KAction* getAction() const;

    //reimp
    QVariant data(int column, int role) const;
signals:
    void execute( QTreeWidgetItem * );
public slots:
    void slotExecute();

private:
  SnippetItem( QTreeWidget *parent, const QString &name, const QString &text );
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
