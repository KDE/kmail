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
 * to the Free Software Foundation, Inc, 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */
#include "kmtopwidget.h"

#include <kapplication.h>

//-----------------------------------------------------------------------------
KMTopLevelWidget::KMTopLevelWidget(const char* aName):
  KMainWindow(0, aName)
{
  kapp->ref();
}


//-----------------------------------------------------------------------------
KMTopLevelWidget::~KMTopLevelWidget()
{
  kapp->deref();
}


//-----------------------------------------------------------------------------
void KMTopLevelWidget::closeEvent(QCloseEvent* e)
{
  // Almost verbatim from KMainWindow
  KMainWindow::closeEvent( e ); 
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
