#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "certificatehandlingdialogimpl.h"
#include "certificatewizardimpl.h"

#include <qlistview.h>
#include <qpopupmenu.h>
#include <qpushbutton.h>
#include <qlabel.h>

#include <klocale.h>
#include <kdebug.h>
/*
 *  Constructs a CertificateHandlingDialogImpl which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
CertificateHandlingDialogImpl::CertificateHandlingDialogImpl( QWidget* parent,  const char* name, WFlags fl )
    : CertificateHandlingDialog( parent, name, fl )
{
}

/*
 *  Destroys the object and frees any allocated resources
 */
CertificateHandlingDialogImpl::~CertificateHandlingDialogImpl()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 * protected slot
 */
void CertificateHandlingDialogImpl::slotDeleteCertificate()
{
    // PENDING(khz) Add code to delete certificate.

    QListViewItem* item = certificatesLV->selectedItem();
    Q_ASSERT( item );
    delete item;
}

/*
 * protected slot
 */
void CertificateHandlingDialogImpl::slotCertificateSelectionChanged( QListViewItem* item )
{
    if( item ) {
        requestPopup->setItemEnabled(1, true);
        requestPopup->setItemEnabled(2, true);
        deletePB->setEnabled( true );
        if( item->text( 2 ) == i18n( "Sign/Encrypt" ) ) {
            useForSigningPB->setEnabled( true );
            useForEncryptingPB->setEnabled( true );
        } else if( item->text( 2 ) == i18n( "Sign" ) ) {
            useForSigningPB->setEnabled( true );
            useForEncryptingPB->setEnabled( false );
        } else if( item->text( 2 ) == i18n( "Encrypt" ) ) {
            useForSigningPB->setEnabled( false );
            useForEncryptingPB->setEnabled( true );
        } else {
            // should not happen, such a certificate would be pretty useless
            useForSigningPB->setEnabled( false );
            useForEncryptingPB->setEnabled( false );
        }
    } else {
        useForSigningPB->setEnabled( false );
        useForEncryptingPB->setEnabled( false );
        requestPopup->setItemEnabled(1, false);
        requestPopup->setItemEnabled(2, true);
        deletePB->setEnabled( false );
    }
}

/*
 * protected slot
 */
void CertificateHandlingDialogImpl::slotRequestChangedCertificate()
{
    // PENDING(khz) Send change request to CA
    kdWarning() << "CertificateHandlingDialogImpl::slotRequestChangedCertificate() not yet implemented!" << endl;
}

/*
 * protected slot
 */
void CertificateHandlingDialogImpl::slotRequestExtendedCertificate()
{
    // PENDING(khz) Send extension request CA
    kdWarning() << "CertificateHandlingDialogImpl::slotRequestExtendedCertificate() not yet implemented!" << endl;
}

/*
 * protected slot
 */
void CertificateHandlingDialogImpl::slotRequestNewCertificate()
{
    CertificateWizardImpl wizard;
    if( wizard.exec() == QDialog::Accepted ) {
        // PENDING(khz) Handle the created certificates.

        // Insert a dummy certificate.
        // PENDING(khz) Remove this code.
        new QListViewItem( certificatesLV, "BlahCertificate", "0x58643BFE", i18n( "Sign/Encrypt" ) );
    }
}

/*
 * protected slot
 */
void CertificateHandlingDialogImpl::slotUseForEncrypting()
{
    QListViewItem* item = certificatesLV->selectedItem();
    Q_ASSERT( item );
    if( item ) {
        // show the used certificate in label
        encryptCertLA->setText( item->text( 0 ) );

        // iterate over the listview and reset all usage markings
        QListViewItemIterator it( certificatesLV );
        QListViewItem* current;
        while( ( current = it.current() ) ) {
            if( current->text( 3 ) == i18n( "Sign/Encrypt" ) )
                current->setText( 3, i18n( "Sign" ) );
            else if( current->text( 3 ) == i18n( "Encrypt" ) )
                current->setText( 3, "" );
            ++it;
        }

        // mark the current one as used
        if( item->text( 3 ) == i18n( "Sign" ) )
            item->setText( 3, i18n( "Sign/Encrypt" ) );
        else if( item->text( 3 ).isEmpty() )
            item->setText( 3, i18n( "Encrypt" ) );
    }
}

/*
 * protected slot
 */
void CertificateHandlingDialogImpl::slotUseForSigning()
{
    QListViewItem* item = certificatesLV->selectedItem();
    Q_ASSERT( item );
    if( item ) {
        // show the used certificate in label
        signCertLA->setText( item->text( 0 ) );

        // iterate over the listview and reset all usage markings
        QListViewItemIterator it( certificatesLV );
        QListViewItem* current;
        while( ( current = it.current() ) ) {
            ++it;
            if( current->text( 3 ) == i18n( "Sign/Encrypt" ) )
                current->setText( 3, i18n( "Encrypt" ) );
            else if( current->text( 3 ) == i18n( "Sign" ) )
                current->setText( 3, "" );
        }

        // mark the current one as used
        if( item->text( 3 ) == i18n( "Encrypt" ) )
            item->setText( 3, i18n( "Sign/Encrypt" ) );
        else if( item->text( 3 ).isEmpty() )
            item->setText( 3, i18n( "Sign" ) );
    }
}


#include "certificatehandlingdialogimpl.moc"
