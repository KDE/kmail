#ifndef DIRECTORYSERVICESCONFIGURATIONDIALOGIMPL_H
#define DIRECTORYSERVICESCONFIGURATIONDIALOGIMPL_H
#include "directoryservicesconfigurationdialog.h"

class CryptPlugWrapper;

class DirectoryServicesConfigurationDialogImpl : public DirectoryServicesConfigurationDialog
{
    Q_OBJECT

public:
    DirectoryServicesConfigurationDialogImpl( QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    ~DirectoryServicesConfigurationDialogImpl();

    void enableDisable( CryptPlugWrapper* wrapper );
    
protected slots:
    void slotServiceChanged( QListViewItem* );
    void slotServiceSelected( QListViewItem* );
    void slotAddService();
    void slotDeleteService();
};

#endif // DIRECTORYSERVICESCONFIGURATIONDIALOGIMPL_H
