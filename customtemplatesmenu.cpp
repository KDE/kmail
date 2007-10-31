/*   -*- mode: C++; c-file-style: "gnu" -*-
*   kmail: KDE mail client
*   This file: Copyright (C) 2006 Dmitry Morozhnikov
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/

#include "customtemplatesmenu.h"

#include <QSignalMapper>

#include <klocale.h>
#include <kactionmenu.h>
#include <kmenu.h>
#include <kactioncollection.h>

#include "customtemplates.h"
#include "customtemplates_kfg.h"
#include "globalsettings.h"
#include "kmkernel.h"


CustomTemplatesMenu::CustomTemplatesMenu(QWidget *owner,KActionCollection *ac)
{
  mOwnerActionCollection = ac;

  mCustomForwardActionMenu = new KActionMenu( KIcon( "mail_custom_forward" ),
                                              i18n("With Custom Template"), owner );
  mOwnerActionCollection->addAction( "custom_forward", mCustomForwardActionMenu );

  mCustomReplyActionMenu = new KActionMenu( KIcon( "mail_custom_reply" ),
                                            i18n("Reply With Custom Template"), owner );
  mOwnerActionCollection->addAction( "custom_reply", mCustomReplyActionMenu );

  mCustomReplyAllActionMenu = new KActionMenu( KIcon( "mail_custom_reply_all" ),
                                               i18n("Reply to All With Custom Template"), owner );
  mOwnerActionCollection->addAction( "custom_reply_all", mCustomReplyAllActionMenu );

  mCustomForwardMapper = new QSignalMapper( this );
  connect( mCustomForwardMapper, SIGNAL(mapped( int )),
           this, SLOT(slotForwardSelected( int )) );

  mCustomReplyMapper = new QSignalMapper( this );
  connect( mCustomReplyMapper, SIGNAL(mapped( int )),
           this, SLOT(slotReplySelected( int )) );

  mCustomReplyAllMapper = new QSignalMapper( this );
  connect( mCustomReplyAllMapper, SIGNAL(mapped( int )),
           this, SLOT(slotReplyAllSelected( int )) );

  connect( kmkernel, SIGNAL(customTemplatesChanged()), this, SLOT(update()) );

  update();
}


CustomTemplatesMenu::~CustomTemplatesMenu()
{
  clear();

  delete mCustomReplyActionMenu;
  delete mCustomReplyAllActionMenu;
  delete mCustomForwardActionMenu;

  delete mCustomReplyMapper;
  delete mCustomReplyAllMapper;
  delete mCustomForwardMapper;
}


void CustomTemplatesMenu::clear()
{
  if ( !mCustomTemplateActions.isEmpty() ) {
    for ( QList<KAction*>::iterator ait = mCustomTemplateActions.begin();
          ait != mCustomTemplateActions.end() ; ++ait ) {
      KAction *action = (*ait);
      mCustomReplyMapper->removeMappings( action );
      mCustomReplyAllMapper->removeMappings( action );
      mCustomForwardMapper->removeMappings( action );
      delete action;
    }
    mCustomTemplateActions.clear();
  }

  mCustomReplyActionMenu->menu()->clear();
  mCustomReplyAllActionMenu->menu()->clear();
  mCustomForwardActionMenu->menu()->clear();
  mCustomTemplates.clear();
}


void CustomTemplatesMenu::update()
{
  //kDebug(5006) << "updating";

  clear();

  QStringList list = GlobalSettingsBase::self()->customTemplates();
  QStringList::iterator it = list.begin();
  int idx = 0;
  int replyc = 0;
  int replyallc = 0;
  int forwardc = 0;
  for ( ; it != list.end(); ++it ) {
    CTemplates t( *it );
    mCustomTemplates.append( *it );

    KAction *action;
    switch ( t.type() ) {
    case CustomTemplates::TReply:
      action = new KAction( (*it).replace( "&", "&&" ), mOwnerActionCollection );
      action->setShortcut( t.shortcut() );
      connect( action, SIGNAL(triggered(bool)), mCustomReplyMapper, SLOT(map()) );
      mCustomReplyMapper->setMapping( action, idx );
      mCustomReplyActionMenu->addAction( action );
      mCustomTemplateActions.append( action );
      ++replyc;
      break;
    case CustomTemplates::TReplyAll:
      action = new KAction( (*it).replace( "&", "&&" ), mOwnerActionCollection );
      action->setShortcut( t.shortcut() );
      connect( action, SIGNAL(triggered(bool)), mCustomReplyAllMapper, SLOT(map()) );
      mCustomReplyAllMapper->setMapping( action, idx );
      mCustomReplyAllActionMenu->addAction( action );
      mCustomTemplateActions.append( action );
      ++replyallc;
      break;
    case CustomTemplates::TForward:
      action = new KAction( (*it).replace( "&", "&&" ), mOwnerActionCollection );
      action->setShortcut( t.shortcut() );
      connect( action, SIGNAL(triggered(bool)), mCustomForwardMapper, SLOT(map()) );
      mCustomForwardMapper->setMapping( action, idx );
      mCustomForwardActionMenu->addAction( action );
      mCustomTemplateActions.append( action );
      ++forwardc;
      break;
    case CustomTemplates::TUniversal:
      action = new KAction( (*it).replace( "&", "&&" ), mOwnerActionCollection );
      connect( action, SIGNAL(triggered(bool)), mCustomReplyMapper, SLOT(map()) );
      mCustomReplyMapper->setMapping( action, idx );
      mCustomReplyActionMenu->addAction( action );
      mCustomTemplateActions.append( action );
      ++replyc;
      action = new KAction( (*it).replace( "&", "&&" ), mOwnerActionCollection );
      connect( action, SIGNAL(triggered(bool)), mCustomReplyAllMapper, SLOT(map()) );
      mCustomReplyAllMapper->setMapping( action, idx );
      mCustomReplyAllActionMenu->addAction( action );
      mCustomTemplateActions.append( action );
      ++replyallc;
      action = new KAction( (*it).replace( "&", "&&" ), mOwnerActionCollection );
      connect( action, SIGNAL(triggered(bool)), mCustomForwardMapper, SLOT(map()) );
      mCustomForwardMapper->setMapping( action, idx );
      mCustomForwardActionMenu->addAction( action );
      mCustomTemplateActions.append( action );
      ++forwardc;
      break;
    }

    ++idx;
  }
  if ( !replyc ) {
    QAction *noAction = mCustomReplyActionMenu->menu()->addAction(
        i18n( "(no custom templates)" ) );
    noAction->setEnabled( false );
    mCustomReplyActionMenu->setEnabled( false );
  }
  if ( !replyallc ) {
    QAction *noAction = mCustomReplyAllActionMenu->menu()->addAction(
        i18n( "(no custom templates)" ) );
    noAction->setEnabled( false );
    mCustomReplyAllActionMenu->setEnabled( false );
  }
  if ( !forwardc ) {
    QAction *noAction = mCustomForwardActionMenu->menu()->addAction(
        i18n( "(no custom templates)" ) );
    noAction->setEnabled( false );
    mCustomForwardActionMenu->setEnabled( false );
  }
}


void CustomTemplatesMenu::slotReplySelected( int idx )
{
  //kDebug(5006) << "selected idx=" << idx;
  emit replyTemplateSelected( mCustomTemplates[idx] );
}

void CustomTemplatesMenu::slotReplyAllSelected( int idx )
{
  //kDebug(5006) << "selected idx=" << idx;
  emit replyAllTemplateSelected( mCustomTemplates[idx] );
}

void CustomTemplatesMenu::slotForwardSelected( int idx )
{
  //kDebug(5006) << "selected idx=" << idx;
  emit forwardTemplateSelected( mCustomTemplates[idx] );
}


#include "customtemplatesmenu.moc"
