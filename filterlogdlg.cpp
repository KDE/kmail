/*
    This file is part of KMail.
    Copyright (c) 2003 Andreas Gungl <a.gungl@gmx.de>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "filterlogdlg.h"
#include "filterlog.h"

#include <kdebug.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <qcheckbox.h>
#include <qstringlist.h>
#include <qtextedit.h>
#include <qvbox.h>

#include <errno.h>


using namespace KMail;


FilterLogDialog::FilterLogDialog( QWidget * parent )
: KDialogBase( parent, "FilterLogDlg", false, i18n( "KMail Filter Log Viewer" ),
              User1|User2|Close, Close, true, i18n("Clea&r"), i18n("&Save...") )
{
  setWFlags( WDestructiveClose );
  QVBox *page = makeVBoxMainWidget();
  
  mTextEdit = new QTextEdit( page );
  mTextEdit->setReadOnly( true );
  mTextEdit->setWordWrap( QTextEdit::NoWrap );
  mTextEdit->setTextFormat( QTextEdit::PlainText );

  QStringList logEntries = FilterLog::instance()->getLogEntries();
  for ( QStringList::Iterator it = logEntries.begin(); 
        it != logEntries.end(); ++it ) 
  {
    mTextEdit->append( *it );
  }
  
  mLineWrapBox = new QCheckBox( i18n("&Wrap lines in viewer"), page );
  mLineWrapBox->setChecked( false );
  connect( mLineWrapBox, SIGNAL(clicked()),
            this, SLOT(slotSwitchLineWrap(void)) );

  mLogActiveBox = new QCheckBox( i18n("&Log filter activities"), page );
  mLogActiveBox->setChecked( FilterLog::instance()->isLogging() );
  connect( mLogActiveBox, SIGNAL(clicked()),
            this, SLOT(slotSwitchLogState(void)) );

  connect(FilterLog::instance(), SIGNAL(logEntryAdded(QString)), 
          this, SLOT(slotLogEntryAdded(QString)));
  connect(FilterLog::instance(), SIGNAL(logShrinked(void)), 
          this, SLOT(slotLogShrinked(void)));
  connect(FilterLog::instance(), SIGNAL(logStateChanged(void)), 
          this, SLOT(slotLogStateChanged(void)));
  
  setInitialSize( QSize( 500, 400 ) );
}


void FilterLogDialog::slotLogEntryAdded( QString logEntry )
{
  mTextEdit->append( logEntry );
}


void FilterLogDialog::slotLogShrinked()
{
  mTextEdit->clear();
  QStringList logEntries = FilterLog::instance()->getLogEntries();
  for ( QStringList::Iterator it = logEntries.begin(); 
        it != logEntries.end(); ++it ) 
  {
    mTextEdit->append( *it );
  }
}


void FilterLogDialog::slotLogStateChanged()
{
  mLogActiveBox->setChecked( FilterLog::instance()->isLogging() );
}


void FilterLogDialog::slotSwitchLogState()
{
  FilterLog::instance()->setLogging( mLogActiveBox->isChecked() );
}


void FilterLogDialog::slotSwitchLineWrap()
{
  if ( mLineWrapBox->isChecked() )
    mTextEdit->setWordWrap( QTextEdit::WidgetWidth );
  else
    mTextEdit->setWordWrap( QTextEdit::NoWrap );
}


void FilterLogDialog::slotUser1()
{
  FilterLog::instance()->clear();
  mTextEdit->clear();
}


void FilterLogDialog::slotUser2()
{
  QString fileName;
  KFileDialog fdlg( QString::null, QString::null, this, 0, true );
  
  fdlg.setMode( KFile::File );
  fdlg.setSelection( "kmail-filter.log" );
  fdlg.setOperationMode( KFileDialog::Saving );
  if ( fdlg.exec() )
  {
    fileName = "/home/domino/kmail-filter.log";
    if ( !FilterLog::instance()->saveToFile( fdlg.selectedFile() ) )
    {
      KMessageBox::error( this,
                          i18n( "%1 is detailed error description",
                                "Could not write the file:\n%2" )
                          .arg( QString::fromLocal8Bit( strerror( errno ) ) )
                          .arg( fileName ),
                          i18n( "KMail Error" ) );
    }
  }
}


#include "filterlogdlg.moc"
