// This must be first
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "klistboxdialog.h"
#include <k3listbox.h>
#include <QLabel>
#include <QLayout>
//Added by qt3to4:
#include <QVBoxLayout>

KListBoxDialog::KListBoxDialog( QString& _selectedString,
                                const QString& caption,
                                const QString& labelText,
                                QWidget* parent )
    : KDialog( parent ),
      selectedString( _selectedString )

{
    setCaption( caption );
    setButtons( Ok|Cancel );
    setObjectName( "KListBoxDialog" );
    resize( 400, 180 );

    QWidget *page = new QWidget( this );
    setMainWidget( page );
    QVBoxLayout *topLayout = new QVBoxLayout( page );
    topLayout->setSpacing( spacingHint() );
    topLayout->setMargin( 0 );
    labelAboveLA = new QLabel( page );
    labelAboveLA->setObjectName( "labelAboveLA" );
    labelAboveLA->setText( labelText );

    topLayout->addWidget( labelAboveLA );

    entriesLB = new Q3ListBox( page, "entriesLB" );

    topLayout->addWidget( entriesLB );

    commentBelowLA = new QLabel( page );
    commentBelowLA->setObjectName( "commentBelowLA" );
    commentBelowLA->setText( "" );
    topLayout->addWidget( commentBelowLA );
    commentBelowLA->hide();

    // signals and slots connections
    connect( entriesLB, SIGNAL( highlighted( const QString& ) ),
             this,      SLOT(   highlighted( const QString& ) ) );
    connect( entriesLB, SIGNAL( selected(int) ),
                        SLOT(   accept() ) );
    // buddies
    labelAboveLA->setBuddy( entriesLB );
}

/*
 *  Destroys the object and frees any allocated resources
 */
KListBoxDialog::~KListBoxDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

void KListBoxDialog::setLabelAbove(const QString& label)
{
    labelAboveLA->setText( label );
    if( label.isEmpty() )
        labelAboveLA->hide();
    else
        labelAboveLA->show();
}

void KListBoxDialog::setCommentBelow(const QString& comment)
{
    commentBelowLA->setText( comment );
    if( comment.isEmpty() )
        commentBelowLA->hide();
    else
        commentBelowLA->show();
}



void KListBoxDialog::highlighted( const QString& txt )
{
    selectedString = txt;
}

#include "klistboxdialog.moc"
