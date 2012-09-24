// -*- mode: C++; c-file-style: "gnu" -*-
/**
 *
 * Copyright (c) 2004 David Faure <faure@kde.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */

#include "collectionaclpage.h"

#include <akonadi/collection.h>
#include <kdialog.h>
#include <klocale.h>
#include <kvbox.h>
#include <mailcommon/aclmanager.h>
#include <mailcommon/imapaclattribute.h>

#include <QAction>
#include <QActionEvent>
#include <QHBoxLayout>
#include <QListView>
#include <QPushButton>

/**
 * Unfortunately QPushButton doesn't support to plug in
 * a QAction like QToolButton does, so we have to reimplement it :(
 */
class ActionButton : public QPushButton
{
  public:
    ActionButton( QWidget *parent = 0 )
      : QPushButton( parent ),
        mDefaultAction( 0 )
    {
    }

    void setDefaultAction( QAction *action )
    {
      if ( !actions().contains( action ) ) {
        addAction( action );
        connect( this, SIGNAL(clicked()), action, SLOT(trigger()) );
      }

      setText( action->text() );
      setEnabled( action->isEnabled() );

      mDefaultAction = action;
    }

  protected:
    virtual void actionEvent( QActionEvent *event )
    {
      QAction *action = event->action();
      switch ( event->type() ) {
        case QEvent::ActionChanged:
          if ( action == mDefaultAction )  
            setDefaultAction( mDefaultAction );
            return;
          break;
        default:
          break;
      }

      QPushButton::actionEvent( event );
    }

  private:
    QAction *mDefaultAction;
};

CollectionAclPage::CollectionAclPage( QWidget *parent )
  : CollectionPropertiesPage( parent ),
    mAclManager( new MailCommon::AclManager( this ) ),
    mChanged( false )
{
  setObjectName( QLatin1String( "KMail::CollectionAclPage" ) );

  setPageTitle( i18n( "Access Control" ) );
  init();
}

void CollectionAclPage::init()
{
  QHBoxLayout *layout = new QHBoxLayout( this );

  QListView *view = new QListView;
  layout->addWidget( view );

  view->setAlternatingRowColors( true );
  view->setModel( mAclManager->model() );
  view->setSelectionModel( mAclManager->selectionModel() );

  KVBox *buttonBox = new KVBox;
  buttonBox->setSpacing( KDialog::spacingHint() );
  layout->addWidget( buttonBox );

  ActionButton *button = new ActionButton( buttonBox );
  button->setDefaultAction( mAclManager->addAction() );

  button = new ActionButton( buttonBox );
  button->setDefaultAction( mAclManager->editAction() );

  button = new ActionButton( buttonBox );
  button->setDefaultAction( mAclManager->deleteAction() );

  QWidget *spacer = new QWidget( buttonBox );
  spacer->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Expanding );
}

bool CollectionAclPage::canHandle( const Akonadi::Collection &collection ) const
{
  return collection.hasAttribute<MailCommon::ImapAclAttribute>();
}

void CollectionAclPage::load( const Akonadi::Collection &collection )
{
  mAclManager->setCollection( collection );
}

void CollectionAclPage::save( Akonadi::Collection &collection )
{
  mAclManager->save();

  // The collection dialog expects the changed collection to run
  // its own ItemModifyJob, so make him happy...
  MailCommon::ImapAclAttribute *attribute = mAclManager->collection().attribute<MailCommon::ImapAclAttribute>();
  collection.addAttribute( attribute->clone() ); ;
}

#include "collectionaclpage.moc"
