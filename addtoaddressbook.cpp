/* This file implements the dialog to add an address to the KDE
   addressbook database. 
 *
 * copyright:  (C) Mirko Sucker, 1998, 1999, 2000
 * mail to:    Mirko Sucker <mirko@kde.org>
 * requires:   recent C++-compiler, at least Qt 2.0
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 Boston, MA 02111-1307, USA.
 
 * $Revision$
 */

#include <qlabel.h>
#include <qlistbox.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <qstring.h>
#include <qstringlist.h>
#include <kdialogbase.h>
#include <klocale.h>
#include <kabapi.h>
#include <kmessagebox.h>
#include "kmmainwin.h"
#include "kmkernel.h"
#include "addtoaddressbook.h"

AddToKabDialog::AddToKabDialog(QString url_, KabAPI * api_, QWidget *parent)
  : KDialogBase(parent, "AddToKabDialog", true,
		i18n("Add address to the address book"),
		KDialogBase::User1|KDialogBase::User2|KDialogBase::Cancel,
		KDialogBase::User1, true,
		i18n("Create a new entry..."),
		i18n("Add email address")),
  api(api_),
  url(url_)
{
  QStringList names;
  // -----
  QWidget *mainwidget=new QWidget(this);
  QBoxLayout *layout=new QBoxLayout
    (mainwidget, QBoxLayout::TopToBottom,
     KDialog::marginHint(), KDialog::spacingHint());
  layout->setAutoAdd(true);
  QLabel *headline=new QLabel
    (url + "\n"
     +i18n("Select entry to add email address to or create a new one\n"
	  "using the New button:"),
     mainwidget);
  headline=headline; // shut up the "unused ..." - warning
  list=new QListBox(mainwidget);
  if(api->addressbook()->getListOfNames(&names, false, false)
     ==AddressBook::NoError)
    {
      list->insertStringList(names);
    }
  setMainWidget(mainwidget);
  if(actionButton(User1)!=0)
    {
      connect(actionButton(User1), SIGNAL(clicked()),
	      SLOT(newEntry()));
    }
  if(actionButton(User2)!=0)
    {
      connect(actionButton(User2), SIGNAL(clicked()),
	      SLOT(addToEntry()));
      actionButton(User2)->setEnabled(false);
    }
  connect(list, SIGNAL(highlighted(int)), SLOT(highlighted(int)));
}

AddToKabDialog::~AddToKabDialog()
{
}

void AddToKabDialog::addToEntry()
{
  KabKey key;
  AddressBook::Entry entry;
  QString address;
  // ----- get the selected entries key:
  if(list->currentItem()<0)
    {
      debug("AddToKabDialog::addToEntry: called, but no selection.");
      accept(); // this should not return
      return;
    }
  if(api->addressbook()->getKey(list->currentItem(), key)
	 ==AddressBook::NoError)
    { // ----- get the entry
      if(api->addressbook()->getEntry(key, entry)
	 ==AddressBook::NoError)
	{ // everything works fine:
	  address=url.mid(7, 255);	// chop the "mailto:"
	  address=address.stripWhiteSpace();
	  entry.emails.append(address);
	  if(api->addressbook()->change(key, entry)
	     ==AddressBook::NoError)
	    {
	      debug("AddToKabDialog::addToEntry: changes done.");
	      if(api->save(true)==AddressBook::NoError)
		{
		  debug("AddToKabDialog::addToEntry: "
			"changes done.");
		} else {
		  KMessageBox::information
		    (this,
		     i18n("The changes could not be saved.\n"
			  "Maybe the database is locked by "
			  "another process.\nTry again."),
			 i18n("Error"));
		}
	    } else {
	      KMessageBox::information
		(this,
		 i18n("The entry could not be changed.\n"
		      "Maybe the database is locked by another"
		      " process.\nTry again."),
		 i18n("Error"));
	    }
	} else {
	  KMessageBox::information
	    (this,
	     i18n("Sorry - address database changed meanwhile.\n"
		  "Try again."),
	     i18n("Error"));
	}
    } else {
      KMessageBox::information
	(this,
	 i18n("Sorry - address database changed meanwhile.\n"
	      "Try again."),
	     i18n("Error"));
    }
  accept();
}

void AddToKabDialog::newEntry()
{
  KDialogBase dialog(this, 0, true, "Create new address",
		     KDialogBase::Ok|KDialogBase::Cancel,
		     KDialogBase::Ok,
		     true);
  AddressBook::Entry entry;
  KabKey dummy;
  QString address; // the email address to add
  const QString texts[]={
    i18n("First name:"), i18n("Last name:"), i18n("Email address:") 
      };
  QString *fields[]= { &entry.firstname, &entry.lastname, &address };
  const int Size=sizeof(texts)/sizeof(texts[0]);
  QLineEdit *lineedits[Size];
  int count, row=0;
  QWidget *mainwidget=new QWidget(&dialog);
  QGridLayout layout(mainwidget, 5, 2, KDialog::marginHint(),
		     KDialog::spacingHint());
  QFrame *frame;
  // -----
  layout.addWidget(new QLabel(i18n("Field"), mainwidget), row, 0);
  layout.addWidget(new QLabel(i18n("Content"), mainwidget), row, 1);
  ++row;
  frame=new QFrame(mainwidget);
  frame->setFrameStyle(QFrame::HLine | QFrame::Sunken);
  layout.addMultiCellWidget(frame, row, row, 0, 1);
  ++row;
  for(count=0; count<Size; ++count)
    {
      layout.addWidget(new QLabel(texts[count], mainwidget), row, 0);
      lineedits[count]=new QLineEdit(mainwidget);
      layout.addWidget(lineedits[count], row, 1);
      ++row;
    }
  lineedits[2]->setText(url.mid(0, 255));	// "mailto:" is already chopped
  dialog.setMainWidget(mainwidget);
  dialog.adjustSize();
  if(dialog.exec())
    {
      for(count=0; count<Size; ++count)
	{
	  *fields[count]=lineedits[count]->text();
	}
      entry.emails.append(address);
      if(api->add(entry, dummy)==AddressBook::NoError)
	{
	  debug("AddToKabDialog::newEntry: entry added.");
	  if(api->save(true)==AddressBook::NoError)
	    {
	      debug("AddToKabDialog::newEntry: "
		    "changes saved.");
	      accept();
	    } else {
	      KMessageBox::information
		(this,
		 i18n("The changes could not be saved.\n"
		      "Maybe the database is locked by "
		      "another process.\nTry again."),
		 i18n("Error"));
	    }
	} else {
	  KMessageBox::information
	    (this,
	     i18n("The changes could not be saved.\n"
		  "Maybe the database is locked by "
		  "another process.\nTry again."),
	     i18n("Error"));
	}
    }
}

void AddToKabDialog::highlighted(int)
{
  if(actionButton(User2)!=0)
    {
      actionButton(User2)->setEnabled(true);
    }  
}

  
#include "addtoaddressbook.moc"
