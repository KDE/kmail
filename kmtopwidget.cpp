/*
 * KMTopLevelWidget class implementation
 *
 * Copyright (C) 1998 Stefan Taferner
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABLILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details. You should have received a copy
 * of the GNU General Public License along with this program; if not, write
 * to the Free Software Foundation, Inc, 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 */
#include "kmtopwidget.h"
#include <kapp.h>
#include "kmsender.h"
#include "kmglobal.h"

//-----------------------------------------------------------------------------
KMTopLevelWidget::KMTopLevelWidget(const char* aName):
  KMTopLevelWidgetInherited(aName)
{}


//-----------------------------------------------------------------------------
KMTopLevelWidget::~KMTopLevelWidget()
{}


//-----------------------------------------------------------------------------
void KMTopLevelWidget::closeEvent(QCloseEvent* e)
{
  // Almost verbatim from KTMainWindow
  writeConfig();
  if (queryClose())
  {
    e->accept();

    int not_withdrawn = 0;
    QListIterator<KTMainWindow> it(*KTMainWindow::memberList);
    for (it.toFirst(); it.current(); ++it)
    {
      if ( !it.current()->testWState( WState_ForceHide ) )
        not_withdrawn++;
    }

    if ( not_withdrawn <= 1 ) { // last window close accepted?
      kernel->quit();             // ...and quit aplication.
    }
  }
  else
    e->ignore();
}


//-----------------------------------------------------------------------------
void KMTopLevelWidget::readConfig(void)
{
}


//-----------------------------------------------------------------------------
void KMTopLevelWidget::writeConfig(void)
{
}

#include "kmtopwidget.moc"
