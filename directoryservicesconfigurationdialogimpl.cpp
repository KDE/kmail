#include <config.h>
#include "directoryservicesconfigurationdialogimpl.h"
#include "adddirectoryservicedialogimpl.h"
#include "cryptplugwrapper.h"

#include <qlistview.h>
#include <qpushbutton.h>
#include <klineedit.h>
#include <qbuttongroup.h>

/*
 *  Constructs a DirectoryServicesConfigurationDialogImpl which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
DirectoryServicesConfigurationDialogImpl::DirectoryServicesConfigurationDialogImpl( QWidget* parent,  const char* name, WFlags fl )
    : DirectoryServicesConfigurationDialog( parent, name, fl )
{
    x500LV->setSorting( -1 );
}


/*
 *  Destroys the object and frees any allocated resources
 */
DirectoryServicesConfigurationDialogImpl::~DirectoryServicesConfigurationDialogImpl()
{
    // no need to delete child widgets, Qt does it all for us
}


/**
   Enables or disables the widgets in this dialog according to the
   capabilities of the current plugin passed as a parameter.
*/
void DirectoryServicesConfigurationDialogImpl::enableDisable( CryptPlugWrapper* cryptPlug )
{
    // disable the whole page if the plugin does not support the use
    // of directory services
    setEnabled( cryptPlug->hasFeature( Feature_CertificateDirectoryService ) ||
                cryptPlug->hasFeature( Feature_CRLDirectoryService ) );

    localRemoteCertBG->setEnabled( cryptPlug->hasFeature( Feature_CertificateDirectoryService ) );
    localRemoteCRLBG->setEnabled( cryptPlug->hasFeature( Feature_CRLDirectoryService ) );
}


/*
 * protected slot
 */
void DirectoryServicesConfigurationDialogImpl::slotServiceChanged( QListViewItem* item )
{
    if( item )
        removeServicePB->setEnabled( true );
    else
        removeServicePB->setEnabled( false );
}


/*
 * protected slot
 */
void DirectoryServicesConfigurationDialogImpl::slotServiceSelected( QListViewItem* item )
{
    AddDirectoryServiceDialogImpl* dlg = new AddDirectoryServiceDialogImpl( this );
    dlg->serverNameED->setText( item->text( 0 ) );
    dlg->portED->setText( item->text( 1 ) );
    dlg->descriptionED->setText( item->text( 2 ) );
    if( dlg->exec() == QDialog::Accepted ) {
        item->setText( 0, dlg->serverNameED->text() );
        item->setText( 1, dlg->portED->text() );
        item->setText( 2, dlg->descriptionED->text() );
    }
    delete dlg;
}


/*
 * protected slot
 */
void DirectoryServicesConfigurationDialogImpl::slotAddService()
{
    AddDirectoryServiceDialogImpl* dlg = new AddDirectoryServiceDialogImpl( this );
    if( dlg->exec() == QDialog::Accepted ) {
        (void)new QListViewItem( x500LV, x500LV->lastItem(),
                                 dlg->serverNameED->text(),
                                 dlg->portED->text(),
                                 dlg->descriptionED->text() );
    }
}

/*
 * protected slot
 */
void DirectoryServicesConfigurationDialogImpl::slotDeleteService()
{
    QListViewItem* item = x500LV->selectedItem();
    Q_ASSERT( item );
    if( !item )
        return;
    else
        delete item;
    x500LV->triggerUpdate();
    slotServiceChanged( x500LV->selectedItem() );
}


#include "directoryservicesconfigurationdialogimpl.moc"
