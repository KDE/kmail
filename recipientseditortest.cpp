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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
    
    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include "recipientseditortest.h"

#include "recipientseditor.h"

#include <kaboutdata.h>
#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kmessagebox.h>

#include <qpushbutton.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qtextedit.h>

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
  topLayout->addMultiCellWidget( mRecipients, 1, 1, 0, 1 );

  kdDebug() << "SIZEHINT: " << mRecipients->sizeHint() << endl;

//  mRecipients->setFixedHeight( 10 );
  
  QTextEdit *editor = new QTextEdit( this );
  topLayout->addMultiCellWidget( editor, 2, 2, 0, 1 );
  
  QPushButton *button = new QPushButton( "&Close", this );
  topLayout->addMultiCellWidget( button, 3, 3, 0, 1 );
  connect( button, SIGNAL( clicked() ), SLOT( slotClose() ) );
}

void Composer::slotClose()
{
  QString text;

  text += "<qt>";

  Recipient::List recipients = mRecipients->recipients();
  Recipient::List::ConstIterator it;
  for( it = recipients.begin(); it != recipients.end(); ++it ) {
    text += "<b>" + (*it).typeLabel() + ":</b> " + (*it).email() + "<br/>";
  }

  text += "</qt>";

  KMessageBox::information( this, text );

  close();
}

int main( int argc, char **argv )
{
  KAboutData aboutData( "testrecipienteditor",
   "Test Recipient Editor", "0.1" );
  KCmdLineArgs::init( argc, argv, &aboutData );

  KApplication app;

  QObject::connect( &app, SIGNAL( lastWindowClosed() ), &app, SLOT( quit() ) );

  QWidget *wid = new Composer( 0 );
  
  wid->show();

  return app.exec();
}

#include "recipientseditortest.moc"
