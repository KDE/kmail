#ifndef ADDDIRECTORYSERVICEDIALOGIMPL_H
#define ADDDIRECTORYSERVICEDIALOGIMPL_H
#include "adddirectoryservicedialog.h"

class AddDirectoryServiceDialogImpl : public AddDirectoryServiceDialog
{ 
    Q_OBJECT

public:
    AddDirectoryServiceDialogImpl( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~AddDirectoryServiceDialogImpl();

};

#endif // ADDDIRECTORYSERVICEDIALOGIMPL_H
