#ifndef __KMFOLDERDIA
#define __KMFOLDERDIA

#include <qlined.h>
#include <qpushbt.h>
#include <qlistbox.h>
#include <qdialog.h>
#include "kmaccount.h"

class KMFolderDialog : public QDialog
{
	Q_OBJECT
public:
	KMFolderDialog(QWidget *parent=0,const char *name=0);
	~KMFolderDialog();

	QLineEdit *nameEdit;
	QPushButton *addButton,*removeButton;
	QListBox *assocList,*accountList;
	KMAccountMan *accountMan;
private slots:
	void doAccept();
	void doAdd();
	void doAccountHighlighted(int);
	void doAccountSelected(int);
	void doAssocHighlighted(int);
	void doAssocSelected(int);
	void doRemove();
protected slots:
	void accept();
};

#endif

