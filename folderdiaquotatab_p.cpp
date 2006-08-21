#include "folderdiaquotatab_p.h"

#include <qlayout.h>
#include <qlabel.h>
#include <qprogressbar.h>
#include <qwhatsthis.h>
#include <qcombobox.h>

#include <math.h>

#include "kmkernel.h"
#include "klocale.h"
#include "kconfig.h"
#include "kdebug.h"
#include "kdialog.h"
#include "globalsettings.h"
#include "quotajobs.h"

using namespace KMail;

struct QuotaInfo;

QuotaWidget::QuotaWidget( QWidget* parent, const char* name )
        :QWidget( parent, name )
{
      QVBoxLayout *box = new QVBoxLayout(this);
      QWidget *stuff = new QWidget( this );
      QGridLayout* layout =
          new QGridLayout( stuff, 3, 3,
                           KDialog::marginHint(),
                           KDialog::spacingHint() );
      mInfoLabel = new QLabel("", stuff );
      mRootLabel = new QLabel("", stuff );
      mProgressBar = new QProgressBar( stuff );
      layout->addWidget( new QLabel( i18n("Root:" ), stuff ), 0, 0 );
      layout->addWidget( mRootLabel, 0, 1 );
      layout->addWidget( new QLabel( i18n("Usage:"), stuff ), 1, 0 );
      //layout->addWidget( new QLabel( i18n("Status:"), stuff ), 2, 0 );
      layout->addWidget( mInfoLabel, 1, 1 );
      layout->addWidget( mProgressBar, 2, 1 );
      box->addWidget( stuff );
      box->addStretch( 2 );

      readConfig();
}

void QuotaWidget::setQuotaInfo( const QuotaInfo& info )
{
      // we are assuming only to get STORAGE type info here, thus
      // casting to int is safe
      int current = info.current().toInt();
      int max = info.max().toInt();
      int factor = static_cast<int> ( pow( 1000, mFactor ) );
      mProgressBar->setProgress( current, max );
      mInfoLabel->setText( i18n("%1 of %2 %3 used").arg( current/factor )
                                .arg( max/factor ).arg( mUnits ) );
      mRootLabel->setText( info.root() );
}

void QuotaWidget::readConfig()
{
      if( GlobalSettings::self()->quotaUnit() == GlobalSettings::EnumQuotaUnit::KB )
      {
            mUnits = QString( i18n("KB") );
            mFactor = 0;
      }
      else if( GlobalSettings::self()->quotaUnit() == GlobalSettings::EnumQuotaUnit::MB )
           {
                mUnits = QString( i18n("MB") );
                mFactor = 1;
           }
      else if( GlobalSettings::self()->quotaUnit() == GlobalSettings::EnumQuotaUnit::GB )
           {
               mUnits = QString( i18n("GB") );
               mFactor = 2;
           }
}


#include "folderdiaquotatab_p.moc"
