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

#include <kactioncollection.h>
#include <kcombobox.h>
#include <kkeybutton.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpushbutton.h>
#include <ktextedit.h>

#include <qlabel.h>
#include <qlayout.h>

/*
 *  Constructs a SnippetDlg as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
SnippetDlg::SnippetDlg( KActionCollection *ac, QWidget *parent, const char *name,
                        bool modal, WFlags fl )
  : SnippetDlgBase( parent, name, modal, fl ), actionCollection( ac )
{
  if ( !name ) {
    setName( "SnippetDlg" );
  }

  shortcutLabel = new QLabel( this, "shortcutLabel" );
  shortcutButton = new KKeyButton( this );
  connect( shortcutButton, SIGNAL(capturedShortcut( const KShortcut &)),
           this, SLOT(slotCapturedShortcut(const KShortcut &)) );

  btnAdd->setEnabled( false );
  connect( snippetName, SIGNAL(textChanged(const QString &)),
           this, SLOT(slotTextChanged(const QString &)) );
  connect( snippetName, SIGNAL(returnPressed()),
           this, SLOT(slotReturnPressed()) );

  layout3->addWidget( shortcutLabel, 7, 0 );
  layout3->addWidget( shortcutButton, 7, 1 );

  snippetText->setMinimumSize( 500, 300 );

  // tab order
  setTabOrder( snippetText, shortcutButton );
  setTabOrder( shortcutButton, btnAdd );
  setTabOrder( btnAdd, btnCancel );

  shortcutLabel->setBuddy( shortcutButton );
  languageChange();
}

/*
 *  Destroys the object and frees any allocated resources
 */
SnippetDlg::~SnippetDlg()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void SnippetDlg::languageChange()
{
  shortcutLabel->setText( i18n( "Sh&ortcut:" ) );
}

static bool shortcutIsValid( const KActionCollection* actionCollection, const KShortcut &sc )
{
  KActionPtrList actions = actionCollection->actions();
  KActionPtrList::Iterator it( actions.begin() );
  for ( ; it != actions.end(); it++ ) {
    if ( (*it)->shortcut() == sc ) return false;
  }
  return true;
}

void SnippetDlg::slotCapturedShortcut( const KShortcut& sc )
{

  if ( sc == shortcutButton->shortcut() ) {
    return;
  }
  if ( sc.toString().isNull() ) {
    // null is fine, that's reset, but sc.іsNull() will be false :/
    shortcutButton->setShortcut( KShortcut::null(), false );
  } else {
    if ( !shortcutIsValid( actionCollection, sc ) ) {
      QString msg( i18n( "The selected shortcut is already used, "
                         "please select a different one." ) );
      KMessageBox::sorry( this, msg );
    } else {
      shortcutButton->setShortcut( sc, false );
    }
  }
}

void SnippetDlg::setShowShortcut( bool show )
{
  shortcutLabel->setShown( show );
  shortcutButton->setShown( show );
}

void SnippetDlg::slotTextChanged( const QString &text )
{
  btnAdd->setEnabled( !text.isEmpty() );
}

void SnippetDlg::slotReturnPressed()
{
  if ( !snippetName->text().isEmpty() ) {
    accept();
  }
}

void SnippetDlg::setGroupMode( bool groupMode )
{
  const bool full = !groupMode;
  textLabelGroup->setShown( full );
  cbGroup->setShown( full );
  textLabel2->setShown( full );
  snippetText->setShown( full );
  setShowShortcut( !groupMode );
  if ( groupMode ) {
    resize( width(), 20 );
  }
}

#include "snippetdlg.moc"
