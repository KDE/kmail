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
#include <kdeversion.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <qcheckbox.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qstringlist.h>
#include <qtextedit.h>
#include <qvbox.h>
#include <qwhatsthis.h>

#include <errno.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

using namespace KMail;


FilterLogDialog::FilterLogDialog( QWidget * parent )
: KDialogBase( parent, "FilterLogDlg", false, i18n( "Filter Log Viewer" ),
              User1|User2|Close, Close, true, i18n("Clea&r"), i18n("&Save...") )
{
  setWFlags( WDestructiveClose );
  QVBox *page = makeVBoxMainWidget();

  mTextEdit = new QTextEdit( page );
  mTextEdit->setReadOnly( true );
  mTextEdit->setWordWrap( QTextEdit::NoWrap );
  mTextEdit->setTextFormat( QTextEdit::LogText );

  QStringList logEntries = FilterLog::instance()->getLogEntries();
  for ( QStringList::Iterator it = logEntries.begin();
        it != logEntries.end(); ++it )
  {
    mTextEdit->append( *it );
  }

  mLogRuleEvaluationBox = new QCheckBox( i18n("Log filter &rule evaluation"), page );
  mLogRuleEvaluationBox->setChecked(
      FilterLog::instance()->isContentTypeEnabled( FilterLog::ruleResult ) );
  connect( mLogRuleEvaluationBox, SIGNAL(clicked()),
            this, SLOT(slotSwitchLogRuleEvaluation(void)) );
  QWhatsThis::add( mLogRuleEvaluationBox, 
      i18n( "You can control the feedback in the log concerning the "
            "evaluation of the filter rules of applied filters: "
            "having this option checked will give detailed feedback "
            "for each single filter rule; alternatively, only "
            "feedback about the result of the evaluation of all rules "
            "of a single filter will be given." ) );

  mLogActiveBox = new QCheckBox( i18n("&Log filter activities"), page );
  mLogActiveBox->setChecked( FilterLog::instance()->isLogging() );
  connect( mLogActiveBox, SIGNAL(clicked()),
            this, SLOT(slotSwitchLogState(void)) );
  QWhatsThis::add( mLogActiveBox, 
      i18n( "You can turn logging of filter activities on and off here. "
            "Of course, log data is collected and shown only when logging "
            "is turned on. " ) );

  QHBox * hbox = new QHBox( page );
  new QLabel( i18n("Log size limit:"), hbox );
  mLogMemLimitSpin = new QSpinBox( hbox );
  mLogMemLimitSpin->setMinValue( 1 );
  mLogMemLimitSpin->setMaxValue( 1024 * 256 ); // 256 MB
  // value in the QSpinBox is in KB while it's in Byte in the FilterLog
  mLogMemLimitSpin->setValue( FilterLog::instance()->getMaxLogSize() / 1024 );
  mLogMemLimitSpin->setSuffix( " KB" );
  mLogMemLimitSpin->setSpecialValueText( i18n("unlimited") );
  connect( mLogMemLimitSpin, SIGNAL(valueChanged(int)),
            this, SLOT(slotChangeLogMemLimit(int)) );
  QWhatsThis::add( mLogMemLimitSpin, 
      i18n( "Collecting log data uses memory to temporarily store the "
	    "log data; here you can limit the maximum amount of memory "
	    "to be used: if the size of the collected log data exceeds "
	    "this limit then the oldest data will be discarded until "
	    "the limit is no longer exceeded. " ) );

  connect(FilterLog::instance(), SIGNAL(logEntryAdded(QString)),
          this, SLOT(slotLogEntryAdded(QString)));
  connect(FilterLog::instance(), SIGNAL(logShrinked(void)),
          this, SLOT(slotLogShrinked(void)));
  connect(FilterLog::instance(), SIGNAL(logStateChanged(void)),
          this, SLOT(slotLogStateChanged(void)));

  setInitialSize( QSize( 500, 400 ) );
#if !KDE_IS_VERSION( 3, 2, 91 )
  // HACK - KWin keeps all dialogs on top of their mainwindows, but that's probably
  // wrong (#76026), and should be done only for modals. CVS HEAD should get
  // proper fix in KWin (see also kmfldsearch.cpp)
  XDeleteProperty( qt_xdisplay(), winId(), XA_WM_TRANSIENT_FOR );
#endif
}


void FilterLogDialog::slotLogEntryAdded( QString logEntry )
{
  mTextEdit->append( logEntry );
}


void FilterLogDialog::slotLogShrinked()
{
  // limit the size of the shown log lines as soon as
  // the log has reached it's memory limit
  if ( mTextEdit->maxLogLines() == -1 )
    mTextEdit->setMaxLogLines( mTextEdit->lines() );
}


void FilterLogDialog::slotLogStateChanged()
{
  mLogActiveBox->setChecked( FilterLog::instance()->isLogging() );
  mLogRuleEvaluationBox->setChecked(
      FilterLog::instance()->isContentTypeEnabled( FilterLog::ruleResult ) );

  int newLogSize = FilterLog::instance()->getMaxLogSize() / 1024;
  if ( mLogMemLimitSpin->value() != newLogSize )
    mLogMemLimitSpin->setValue( newLogSize );
}


void FilterLogDialog::slotSwitchLogRuleEvaluation()
{
  if ( mLogRuleEvaluationBox->isChecked() )
    FilterLog::instance()->enableContentType( FilterLog::ruleResult );
  else
    FilterLog::instance()->disableContentType( FilterLog::ruleResult );
}


void FilterLogDialog::slotSwitchLogState()
{
  FilterLog::instance()->setLogging( mLogActiveBox->isChecked() );
}


void FilterLogDialog::slotChangeLogMemLimit( int value )
{
  FilterLog::instance()->setMaxLogSize( value * 1024 );
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
    fileName = fdlg.selectedFile();
    if ( !FilterLog::instance()->saveToFile( fileName ) )
    {
      KMessageBox::error( this,
                          i18n( "Could not write the file %1:\n"
                                "\"%2\" is the detailed error description." )
                          .arg( fileName,
                                QString::fromLocal8Bit( strerror( errno ) ) ),
                          i18n( "KMail Error" ) );
    }
  }
}


#include "filterlogdlg.moc"
