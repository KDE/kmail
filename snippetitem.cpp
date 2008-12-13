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

#include "snippetitem.h"

#include <kaction.h>

#include <qstring.h>

SnippetItem::SnippetItem(QListView * parent, QString name, QString text )
			: QListViewItem( parent, name ), action(0)
{
  strName = name;
  strText = text;
  iParent = -1;
}

SnippetItem::SnippetItem(QListViewItem * parent, QString name, QString text)
			: QListViewItem( parent, name ), action(0)
{
  strName = name;
  strText = text;
  iParent = ((SnippetGroup *)parent)->getId();
}

SnippetItem::~SnippetItem()
{
    if ( action ) {
        action->unplugAll();
        delete action;
    }
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


/*!
    \fn SnippetItem::setText(QString text)
 */
void SnippetItem::setText(QString text)
{
  strText = text;
}


/*!
    \fn SnippetItem::setName(QString name)
 */
void SnippetItem::setName(QString name)
{
  strName = name;
}

void SnippetItem::resetParent()
{
  SnippetGroup * group = dynamic_cast<SnippetGroup*>(parent());
  if (group)
    iParent = group->getId();
}


KAction* SnippetItem::getAction()
{
    return action;
}

void SnippetItem::setAction(KAction * anAction)
{
    action = anAction;
}

void SnippetItem::slotExecute()
{
    emit execute( this );
}


SnippetItem * SnippetItem::findItemByName(QString name, QPtrList<SnippetItem> &list)
{
  for ( SnippetItem * item = list.first(); item; item = list.next() ) {  //write the snippet-list
    if (item->getName() == name)
        return item;
  }
  return NULL;
}

SnippetGroup * SnippetItem::findGroupById(int id, QPtrList<SnippetItem> &list)
{
  for ( SnippetItem * item = list.first(); item; item = list.next() ) {  //write the snippet-list
    SnippetGroup * group = dynamic_cast<SnippetGroup*>(item);
    if (group && group->getId() == id)
        return group;
  }
  return NULL;
}


/* * * * * * * * * * * * * * * * * * * *
Deklaration for class SnippetGroup
* * * * * * * * * * * * * * * * * * * */

int SnippetGroup::iMaxId = 1;

SnippetGroup::SnippetGroup(QListView * parent, QString name, int id)
 : SnippetItem(parent, name, i18n("GROUP"))
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

#include "snippetitem.moc"
