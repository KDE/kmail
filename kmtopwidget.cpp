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


//-----------------------------------------------------------------------------
KMTopLevelWidget::KMTopLevelWidget(const char* aName):
  KMTopLevelWidgetInherited(aName)
{
  initMetaObject();
}


//-----------------------------------------------------------------------------
KMTopLevelWidget::~KMTopLevelWidget()
{
}


//-----------------------------------------------------------------------------
void KMTopLevelWidget::closeEvent(QCloseEvent* e)
{
  KMTopLevelWidgetInherited::closeEvent(e);

  if (e->isAccepted())
  {
    writeConfig();
    e->ignore();
    delete this;
  }
}


//-----------------------------------------------------------------------------
bool KMTopLevelWidget::close(bool aForceKill)
{
  static bool rc;
  rc = KMTopLevelWidgetInherited::close(aForceKill);
  if (!rc) return FALSE;

  if (KTopLevelWidget::memberList && KTopLevelWidget::memberList->isEmpty())
    kapp->quit();

  return TRUE;
}


//-----------------------------------------------------------------------------
void KMTopLevelWidget::forEvery(KForEvery func)
{
  KMTopLevelWidget* w;

  if (KTopLevelWidget::memberList)
  {
    for (w = (KMTopLevelWidget*)KTopLevelWidget::memberList->first(); w;
	 w = (KMTopLevelWidget*)KTopLevelWidget::memberList->next())
    {
      if (w->inherits("KMTopLevelWidget")) (w->*func)();
    }
  } 
}


//-----------------------------------------------------------------------------
void KMTopLevelWidget::readConfig(void)
{
}


//-----------------------------------------------------------------------------
void KMTopLevelWidget::writeConfig(void)
{
}


//-----------------------------------------------------------------------------
#include "kmtopwidget.moc"
