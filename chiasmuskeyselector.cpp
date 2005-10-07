#include "chiasmuskeyselector.h"

#include <klineedit.h>
#include <klistbox.h>
#include <klocale.h>

#include <qlayout.h>
#include <qlabel.h>
//Added by qt3to4:
#include <QVBoxLayout>

ChiasmusKeySelector::ChiasmusKeySelector( QWidget* parent, const QString& caption,
                                          const QStringList& keys, const QString& currentKey,
                                          const QString& lastOptions )
  : KDialogBase( parent, "chiasmusKeySelector", true, caption, Ok|Cancel, Ok, true )
{
  QWidget *page = makeMainWidget();

  QVBoxLayout *layout = new QVBoxLayout(page, KDialog::spacingHint());

  mLabel = new QLabel( i18n( "Please select the Chiasmus key file to use:" ), page );
  layout->addWidget( mLabel );

  mListBox = new KListBox( page );
  mListBox->insertStringList( keys );
  const int current = keys.findIndex( currentKey );
  mListBox->setSelected( qMax( 0, current ), true );
  mListBox->ensureCurrentVisible();
  layout->addWidget( mListBox, 1 );

  QLabel* optionLabel = new QLabel( i18n( "Additional arguments for chiasmus:" ), page );
  layout->addWidget( optionLabel );

  mOptions = new KLineEdit( lastOptions, page );
  optionLabel->setBuddy( mOptions );
  layout->addWidget( mOptions );

  layout->addStretch();

  connect( mListBox, SIGNAL( doubleClicked( Q3ListBoxItem * ) ), this, SLOT( slotOk() ) );
  connect( mListBox, SIGNAL( returnPressed( Q3ListBoxItem * ) ), this, SLOT( slotOk() ) );

  mListBox->setFocus();
}

QString ChiasmusKeySelector::key() const
{
  return mListBox->currentText();
}

QString ChiasmusKeySelector::options() const
{
  return mOptions->text();
}


#include "chiasmuskeyselector.moc"
