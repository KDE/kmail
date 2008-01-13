/*
 *  File : snippetitem.cpp
 *
 *  Author: Robert Gruber <rgruber@users.sourceforge.net>
 *
 *  Copyright: See COPYING file that comes with this distribution
 */

#include <qstring.h>


#include "snippetitem.h"

SnippetItem::SnippetItem( QTreeWidget *parent, const QString &name, const QString &text )
 : QTreeWidgetItem( parent, QStringList(name) )
{
  strName = name;
  strText = text;
  iParent = -1;
}

SnippetItem::SnippetItem( QTreeWidgetItem *parent, const QString &name, const QString &text )
 : QTreeWidgetItem( parent, QStringList(name) )
{
  strName = name;
  strText = text;
  iParent = ((SnippetGroup *)parent)->getId();
}

SnippetItem::~SnippetItem()
{
}


/*!
    \fn SnippetItem::getName()
 */
QString SnippetItem::getName()
{
  return strName;
}


/*!
    \fn SnippetItem::getText
 */
QString SnippetItem::getText()
{
  return strText;
}

void SnippetItem::setText(const QString &text)
{
  strText = text;
}

void SnippetItem::setName(const QString &name)
{
  strName = name;
}

void SnippetItem::resetParent()
{
  SnippetGroup * group = dynamic_cast<SnippetGroup*>(parent());
  if (group)
    iParent = group->getId();
}

SnippetItem * SnippetItem::findItemByName(const QString &name, const QList<SnippetItem *> &list)
{
  foreach ( SnippetItem *const item, list ) {  //write the snippet-list
    if (item->getName() == name)
        return item;
  }
  return NULL;
}

SnippetGroup * SnippetItem::findGroupById(int id, const QList<SnippetItem *> &list)
{
  foreach ( SnippetItem *const item, list ) {  //write the snippet-list
    SnippetGroup * group = dynamic_cast<SnippetGroup*>(item);
    if (group && group->getId() == id)
        return group;
  }
  return NULL;
}

QVariant SnippetItem::data(int column, int role)
{
  if (role == Qt::ToolTipRole && column == 0) {
    //TODO also ask the view if we are supposed to show tooltips...
    return getText();
  }
  return QTreeWidgetItem::data(column, role);
}

/* * * * * * * * * * * * * * * * * * * *
Deklaration for class SnippetGroup
* * * * * * * * * * * * * * * * * * * */

int SnippetGroup::iMaxId = 1;

SnippetGroup::SnippetGroup( QTreeWidget *parent, const QString &name, int id )
 : SnippetItem( parent, name, "GROUP" )
{
    if (id > 0) {
      iId = id;
      if (id >= iMaxId)
        iMaxId = id+1;
    } else {
      iId = iMaxId;
      iMaxId++;
    }
}

SnippetGroup::~SnippetGroup()
{
}

void SnippetGroup::setId(int id)
{
    iId = id; 
    if (iId >= iMaxId)
        iMaxId = iId+1;
}
