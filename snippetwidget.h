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

#ifndef __SNIPPETWIDGET_H__
#define __SNIPPETWIDGET_H__

#include <QTreeView>

namespace MailCommon {
class SnippetsManager;
}

class KActionCollection;
class KMComposerEditor;

class QContextMenuEvent;

/**
 * @author Robert Gruber
 */
class SnippetWidget : public QTreeView
{
  public:
    SnippetWidget( KMComposerEditor *editor, KActionCollection *actionCollection, QWidget *parent = 0 );
    ~SnippetWidget();

  protected:
    virtual void contextMenuEvent( QContextMenuEvent* );
  virtual void dropEvent( QDropEvent * );
  private:
    MailCommon::SnippetsManager *mSnippetsManager;
};

#endif
