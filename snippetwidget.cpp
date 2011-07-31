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

#include "snippetwidget.h"

#include "kmcomposereditor.h"

#include <kactioncollection.h>
#include <klocale.h>
#include <kmenu.h>
#include <mailcommon/snippetsmanager.h>

#include <QtGui/QContextMenuEvent>
#include <QtGui/QHeaderView>

SnippetWidget::SnippetWidget( KMComposerEditor *editor, KActionCollection *actionCollection, QWidget *parent )
  : QTreeView( parent )
{
  header()->hide();
  setAcceptDrops( true );
  setDragEnabled( true );
  setRootIsDecorated( true );
  setAlternatingRowColors( true );

  mSnippetsManager = new MailCommon::SnippetsManager( actionCollection, this );
  mSnippetsManager->setEditor( editor, "insertPlainText", SIGNAL(insertSnippet()) );

  setModel( mSnippetsManager->model() );
  setSelectionModel( mSnippetsManager->selectionModel() );

  connect( this, SIGNAL(activated(QModelIndex)),
           mSnippetsManager->editSnippetAction(), SLOT(trigger()) );
  connect( mSnippetsManager->model(), SIGNAL(rowsInserted(QModelIndex,int,int)),
           this, SLOT(expandAll()) );
  connect( mSnippetsManager->model(), SIGNAL(rowsRemoved(QModelIndex,int,int)),
           this, SLOT(expandAll()) );

  expandAll();
}

SnippetWidget::~SnippetWidget()
{
}

void SnippetWidget::contextMenuEvent( QContextMenuEvent *event )
{
  KMenu popup;

  const bool itemSelected = mSnippetsManager->selectionModel()->hasSelection();

  bool canAddGroup = true;
  bool canAddSnippet = true;
  if ( itemSelected ) {
    popup.addTitle( mSnippetsManager->selectedName() );
    if ( mSnippetsManager->snippetGroupSelected() ) {
      canAddGroup = false;  // subgroups are not permitted
      popup.addAction( mSnippetsManager->editSnippetGroupAction() );
      popup.addAction( mSnippetsManager->deleteSnippetGroupAction() );
    } else {
      canAddSnippet = false; // subsnippets are not permitted
      popup.addAction( mSnippetsManager->addSnippetAction() );
      popup.addAction( mSnippetsManager->editSnippetAction() );
      popup.addAction( mSnippetsManager->deleteSnippetAction() );
    }
    popup.addSeparator();
  } else {
    popup.addTitle( i18n( "Text Snippets" ) );
  }
  if ( canAddSnippet ) {
    popup.addAction( mSnippetsManager->addSnippetAction() );
  }
  if ( canAddGroup ) {
    popup.addAction( mSnippetsManager->addSnippetGroupAction() );
  }

  popup.exec( event->globalPos() );
}

