/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions respectively slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void CertificateHandlingDialog::init()
{
    requestPopup = new QPopupMenu(this);
    requestPopup->insertItem(i18n("New"), 0);
    requestPopup->insertItem(i18n("Extension"), 1);
    requestPopup->insertItem(i18n("Change"), 2);
    
    requestPB->setPopup(requestPopup);
}

void CertificateHandlingDialog::slotDeleteCertificate()
{

}

void CertificateHandlingDialog::slotCertificateSelectionChanged( QListViewItem * )
{

}

void CertificateHandlingDialog::slotRequestChangedCertificate()
{

}

void CertificateHandlingDialog::slotRequestExtendedCertificate()
{

}

void CertificateHandlingDialog::slotRequestNewCertificate()
{

}

void CertificateHandlingDialog::slotUseForEncrypting()
{

}

void CertificateHandlingDialog::slotUseForSigning()
{

}


void CertificateHandlingDialog::slotRequestMenu( int id )
{
    switch (id)
    {
    case 0:
	slotRequestNewCertificate();
	break;
    case 1:
	slotRequestExtendedCertificate();
	break;
    case 2:
	slotRequestChangedCertificate();
	break;
    };
}
