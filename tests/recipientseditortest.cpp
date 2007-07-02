/*
    This file is part of KMail.

    Copyright (c) 2004 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
    
    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include "config.h"

#include "recipientseditortest.h"

#include "recipientseditor.h"

#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kmessagebox.h>
#include "aboutdata.h"

#include <QPushButton>
#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <q3textedit.h>
//Added by qt3to4:
#include <QGridLayout>

Composer::Composer( QWidget *parent )
  : QWidget( parent )
{
  QGridLayout *topLayout = new QGridLayout( this );
  topLayout->setMargin( 4 );
  topLayout->setSpacing( 4 );
  
  QLabel *label = new QLabel( "From:", this );
  topLayout->addWidget( label, 0, 0 );
  QLineEdit *edit = new QLineEdit( this );
  topLayout->addWidget( edit, 0, 1 );
  
  mRecipients = new RecipientsEditor( this );
  topLayout->addWidget( mRecipients, 1, 0, 1, 2 );

  kDebug(5006) << "SIZEHINT: " << mRecipients->sizeHint() << endl;

//  mRecipients->setFixedHeight( 10 );
  
  Q3TextEdit *editor = new Q3TextEdit( this );
  topLayout->addWidget( editor, 2, 0, 1, 2 );
  topLayout->setRowStretch( 2, 1 );
  
  QPushButton *button = new QPushButton( "&Close", this );
  topLayout->addWidget( button, 3, 0, 1, 2 );
  connect( button, SIGNAL( clicked() ), SLOT( slotClose() ) );
}

void Composer::slotClose()
{
#if 0
  QString text;

  text += "<qt>";

  Recipient::List recipients = mRecipients->recipients();
  Recipient::List::ConstIterator it;
  for( it = recipients.begin(); it != recipients.end(); ++it ) {
    text += "<b>" + (*it).typeLabel() + ":</b> " + (*it).email() + "<br/>";
  }

  text += "</qt>";

  KMessageBox::information( this, text );
#endif

  close();
}

int main( int argc, char **argv )
{
  KAboutData aboutData( "testrecipienteditor", 0,
   ki18n("Test Recipient Editor"), "0.1" );
  KCmdLineArgs::init( argc, argv, &aboutData );

  KApplication app;

  QObject::connect( &app, SIGNAL( lastWindowClosed() ), &app, SLOT( quit() ) );

  QWidget *wid = new Composer( 0 );
  
  wid->show();

  int ret = app.exec();
  
  delete wid;
  
  return ret;
}

#include "recipientseditortest.moc"
