#include "config.h"

#include "chiasmuskeyselector.h"

#include <klineedit.h>
#include <k3listbox.h>
#include <klocale.h>

#include <QLayout>
#include <QLabel>
#include <QVBoxLayout>

ChiasmusKeySelector::ChiasmusKeySelector( QWidget* parent, const QString& caption,
                                          const QStringList& keys, const QString& currentKey,
                                          const QString& lastOptions )
  : KDialog( parent )
{
  setCaption( caption );
  setButtons( Ok | Cancel );
  setObjectName( "chiasmusKeySelector" );
  QWidget *page = new QWidget( this );
  setMainWidget(page);

  QVBoxLayout *layout = new QVBoxLayout(page);
  layout->setSpacing(KDialog::spacingHint());

  mLabel = new QLabel( i18n( "Please select the Chiasmus key file to use:" ), page );
  layout->addWidget( mLabel );

  mListBox = new K3ListBox( page );
  mListBox->insertStringList( keys );
  const int current = keys.indexOf( currentKey );
  mListBox->setSelected( qMax( 0, current ), true );
  mListBox->ensureCurrentVisible();
  layout->addWidget( mListBox, 1 );

  QLabel* optionLabel = new QLabel( i18n( "Additional arguments for chiasmus:" ), page );
  layout->addWidget( optionLabel );

  mOptions = new KLineEdit( lastOptions, page );
  optionLabel->setBuddy( mOptions );
  layout->addWidget( mOptions );

  layout->addStretch();

  connect( mListBox, SIGNAL( doubleClicked( Q3ListBoxItem * ) ), this, SLOT( accept() ) );
  connect( mListBox, SIGNAL( returnPressed( Q3ListBoxItem * ) ), this, SLOT( accept() ) );

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
