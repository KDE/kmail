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

#include "snippetdlg.h"

#include <kdialog.h>
#include <klocale.h>

#include <qlabel.h>
#include <qlayout.h>
#include <kpushbutton.h>
#include "kactioncollection.h"
#include "kmessagebox.h"

/*
 *  Constructs a SnippetDlg as a child of 'parent', with the
 *  widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
SnippetDlg::SnippetDlg( KActionCollection* ac, QWidget* parent, bool modal,
                        Qt::WindowFlags f )
  : QDialog( parent, f ),
    actionCollection( ac )
{
    setupUi( this );
    setModal( modal );

    keyWidget->setCheckActionList( ac->actions() );

    //TODO tab order in designer!
    setTabOrder( snippetText, keyWidget );
    setTabOrder( keyWidget, btnAdd );
    setTabOrder( btnAdd, btnCancel );
}

/*
 *  Destroys the object and frees any allocated resources
 */
SnippetDlg::~SnippetDlg()
{
    // no need to delete child widgets, Qt does it all for us
}

void SnippetDlg::setGroupMode( bool groupMode )
{
  const bool full = !groupMode;
  textLabelGroup->setVisible( full );
  cbGroup->setVisible( full );
  textLabel->setVisible( full );
  snippetText->setVisible( full );
  keyWidgetLabel->setVisible( full );
  keyWidget->setVisible( full );
  if ( groupMode )
    resize( width(), minimumSizeHint().height() );
}

#include "snippetdlg.moc"
