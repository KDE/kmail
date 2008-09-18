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
#ifndef KMAIL_FILTERLOGDLG_H
#define KMAIL_FILTERLOGDLG_H

#include <kdialog.h>

class KTextEdit;
class QCheckBox;
class QSpinBox;
class QGroupBox;

namespace KMail {

  /**
    @short KMail Filter Log Collector.
    @author Andreas Gungl <a.gungl@gmx.de>

    The filter log dialog allows a continued observation of the 
    filter log of KMail.
  */
  class FilterLogDialog : public KDialog
  {
    Q_OBJECT
    
    public:
      /** constructor */
      FilterLogDialog( QWidget * parent );
    
    protected slots:
      void slotLogEntryAdded( const QString& logEntry );
      void slotLogShrinked();
      void slotLogStateChanged();
      void slotChangeLogDetail();
      void slotSwitchLogState();
      void slotChangeLogMemLimit( int value );
      
      void slotUser1();
      void slotUser2();

  protected:
      KTextEdit * mTextEdit;
      QCheckBox * mLogActiveBox;
      QGroupBox * mLogDetailsBox;
      QCheckBox * mLogPatternDescBox;
      QCheckBox * mLogRuleEvaluationBox;
      QCheckBox * mLogPatternResultBox;
      QCheckBox * mLogFilterActionBox;
      QSpinBox  * mLogMemLimitSpin;
  };

} // namespace KMail

#endif // KMAIL_FILTERLOGDLG_H
