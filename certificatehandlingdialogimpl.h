#ifndef CERTIFICATEHANDLINGDIALOGIMPL_H
#define CERTIFICATEHANDLINGDIALOGIMPL_H
#include "certificatehandlingdialog.h"

class CertificateHandlingDialogImpl : public CertificateHandlingDialog
{ 
    Q_OBJECT

public:
    CertificateHandlingDialogImpl( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    ~CertificateHandlingDialogImpl();

protected slots:
    void slotDeleteCertificate();
    void slotCertificateSelectionChanged( QListViewItem* );
    void slotRequestChangedCertificate();
    void slotRequestExtendedCertificate();
    void slotRequestNewCertificate();
    void slotUseForEncrypting();
    void slotUseForSigning();

};

#endif // CERTIFICATEHANDLINGDIALOGIMPL_H
