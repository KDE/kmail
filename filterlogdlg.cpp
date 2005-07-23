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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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
#include <qvgroupbox.h>

#include <errno.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

using namespace KMail;


FilterLogDialog::FilterLogDialog( QWidget * parent )
: KDialogBase( parent, "FilterLogDlg", false, i18n( "Filter Log Viewer" ),
              User1|User2|Close, Close, true, KStdGuiItem::clear(), KStdGuiItem::saveAs() )
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

  mLogActiveBox = new QCheckBox( i18n("&Log filter activities"), page );
  mLogActiveBox->setChecked( FilterLog::instance()->isLogging() );
  connect( mLogActiveBox, SIGNAL(clicked()),
           this, SLOT(slotSwitchLogState(void)) );
  QWhatsThis::add( mLogActiveBox,
      i18n( "You can turn logging of filter activities on and off here. "
            "Of course, log data is collected and shown only when logging "
            "is turned on. " ) );

  mLogDetailsBox = new QVGroupBox( i18n( "Logging Details" ), page );
  mLogDetailsBox->setEnabled( mLogActiveBox->isChecked() );
  connect( mLogActiveBox, SIGNAL( toggled( bool ) ),
           mLogDetailsBox, SLOT( setEnabled( bool ) ) );

  mLogPatternDescBox = new QCheckBox( i18n("Log pattern description"),
                                      mLogDetailsBox );
  mLogPatternDescBox->setChecked(
      FilterLog::instance()->isContentTypeEnabled( FilterLog::patternDesc ) );
  connect( mLogPatternDescBox, SIGNAL(clicked()),
           this, SLOT(slotChangeLogDetail(void)) );
  // TODO
  //QWhatsThis::add( mLogPatternDescBox,
  //    i18n( "" ) );

  mLogRuleEvaluationBox = new QCheckBox( i18n("Log filter &rule evaluation"),
                                         mLogDetailsBox );
  mLogRuleEvaluationBox->setChecked(
      FilterLog::instance()->isContentTypeEnabled( FilterLog::ruleResult ) );
  connect( mLogRuleEvaluationBox, SIGNAL(clicked()),
           this, SLOT(slotChangeLogDetail(void)) );
  QWhatsThis::add( mLogRuleEvaluationBox,
      i18n( "You can control the feedback in the log concerning the "
            "evaluation of the filter rules of applied filters: "
            "having this option checked will give detailed feedback "
            "for each single filter rule; alternatively, only "
            "feedback about the result of the evaluation of all rules "
            "of a single filter will be given." ) );

  mLogPatternResultBox = new QCheckBox( i18n("Log filter pattern evaluation"),
                                        mLogDetailsBox );
  mLogPatternResultBox->setChecked(
      FilterLog::instance()->isContentTypeEnabled( FilterLog::patternResult ) );
  connect( mLogPatternResultBox, SIGNAL(clicked()),
           this, SLOT(slotChangeLogDetail(void)) );
  // TODO
  //QWhatsThis::add( mLogPatternResultBox,
  //    i18n( "" ) );

  mLogFilterActionBox = new QCheckBox( i18n("Log filter actions"),
                                       mLogDetailsBox );
  mLogFilterActionBox->setChecked(
      FilterLog::instance()->isContentTypeEnabled( FilterLog::appliedAction ) );
  connect( mLogFilterActionBox, SIGNAL(clicked()),
           this, SLOT(slotChangeLogDetail(void)) );
  // TODO
  //QWhatsThis::add( mLogFilterActionBox,
  //    i18n( "" ) );

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

  setInitialSize( QSize( 500, 500 ) );
#if !KDE_IS_VERSION( 3, 2, 91 )
  // HACK - KWin keeps all dialogs on top of their mainwindows, but that's probably
  // wrong (#76026), and should be done only for modals. CVS HEAD should get
  // proper fix in KWin (see also searchwindow.cpp)
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
  mLogPatternDescBox->setChecked(
      FilterLog::instance()->isContentTypeEnabled( FilterLog::patternDesc ) );
  mLogRuleEvaluationBox->setChecked(
      FilterLog::instance()->isContentTypeEnabled( FilterLog::ruleResult ) );
  mLogPatternResultBox->setChecked(
      FilterLog::instance()->isContentTypeEnabled( FilterLog::patternResult ) );
  mLogFilterActionBox->setChecked(
      FilterLog::instance()->isContentTypeEnabled( FilterLog::appliedAction ) );

  // value in the QSpinBox is in KB while it's in Byte in the FilterLog
  int newLogSize = FilterLog::instance()->getMaxLogSize() / 1024;
  if ( mLogMemLimitSpin->value() != newLogSize )
    mLogMemLimitSpin->setValue( newLogSize );
}


void FilterLogDialog::slotChangeLogDetail()
{
  if ( mLogPatternDescBox->isChecked() !=
       FilterLog::instance()->isContentTypeEnabled( FilterLog::patternDesc ) )
    FilterLog::instance()->setContentTypeEnabled( FilterLog::patternDesc,
                                                  mLogPatternDescBox->isChecked() );

  if ( mLogRuleEvaluationBox->isChecked() !=
       FilterLog::instance()->isContentTypeEnabled( FilterLog::ruleResult ) )
    FilterLog::instance()->setContentTypeEnabled( FilterLog::ruleResult,
                                                  mLogRuleEvaluationBox->isChecked() );

  if ( mLogPatternResultBox->isChecked() !=
       FilterLog::instance()->isContentTypeEnabled( FilterLog::patternResult ) )
    FilterLog::instance()->setContentTypeEnabled( FilterLog::patternResult,
                                                  mLogPatternResultBox->isChecked() );

  if ( mLogFilterActionBox->isChecked() !=
       FilterLog::instance()->isContentTypeEnabled( FilterLog::appliedAction ) )
    FilterLog::instance()->setContentTypeEnabled( FilterLog::appliedAction,
                                                  mLogFilterActionBox->isChecked() );
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
