#include "adddirectoryservicedialogimpl.h"

#include <qvalidator.h>
#include <klineedit.h>

/*
 *  Constructs a AddDirectoryServiceDialogImpl which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
AddDirectoryServiceDialogImpl::AddDirectoryServiceDialogImpl( QWidget* parent,  const char* name, bool modal, WFlags fl )
    : AddDirectoryServiceDialog( parent, name, modal, fl )
{
    portED->setValidator( new QIntValidator( 0, 65535, portED ) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
AddDirectoryServiceDialogImpl::~AddDirectoryServiceDialogImpl()
{
    // no need to delete child widgets, Qt does it all for us
}

#include "adddirectoryservicedialogimpl.moc"
