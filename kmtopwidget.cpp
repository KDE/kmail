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
int KMTopLevelWidget::sWindowCount = 0;

//-----------------------------------------------------------------------------
KMTopLevelWidget::KMTopLevelWidget(const char* aName):
  KMTopLevelWidgetInherited(aName)
{
  initMetaObject();
  sWindowCount++;
}


//-----------------------------------------------------------------------------
KMTopLevelWidget::~KMTopLevelWidget()
{
  sWindowCount--;
  if (sWindowCount <= 0) kapp->quit();
}


//-----------------------------------------------------------------------------
void KMTopLevelWidget::closeEvent(QCloseEvent* e)
{
    writeConfig();
    KMTopLevelWidgetInherited::closeEvent(e);

//   if (e->isAccepted())
//   {
//     writeConfig();
//     e->ignore();
//     //delete this;
//   }
}


//-----------------------------------------------------------------------------
// bool KMTopLevelWidget::close(bool aForceKill)
// {
//   static bool rc;
//   rc = KMTopLevelWidgetInherited::close(aForceKill);
//   if (!rc) return FALSE;

//   if (KMTopLevelWidgetInherited::memberList &&
//       KMTopLevelWidgetInherited::memberList->isEmpty())
//     kapp->quit();

//   return TRUE;
// }


//-----------------------------------------------------------------------------
void KMTopLevelWidget::forEvery(KForEvery func)
{
  KMTopLevelWidget* w;

  if (KMTopLevelWidgetInherited::memberList)
  {
    for (w=(KMTopLevelWidget*)KMTopLevelWidgetInherited::memberList->first();
	 w;
	 w=(KMTopLevelWidget*)KMTopLevelWidgetInherited::memberList->next())
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

bool KMTopLevelWidget::queryExit()
{
  // sven - against quit while sending
  if (msgSender && msgSender->sending()) // sender working?
  {
    msgSender->quitWhenFinished();       // tell him to quit app when finished
    return false;                        // don't quit now
  }
  return true;                           // sender not working, quit
}

#include "kmtopwidget.moc"
