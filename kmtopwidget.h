/*
 * KMTopLevelWidget class interface
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
#ifndef kmtopwidget_h
#define kmtopwidget_h

#include <ktmainwindow.h>

class KMTopLevelWidget;

// easier declarations of function prototypes for forEvery type functions
typedef void (KMTopLevelWidget::*KForEvery)(void);

/** Top level window that offers methods to be called on every
 * existing top level window.
 */
#define KMTopLevelWidgetInherited KTMainWindow
class KMTopLevelWidget: public KTMainWindow
{
  Q_OBJECT

public:
  KMTopLevelWidget(const char *name = 0);
  virtual ~KMTopLevelWidget();

  /** Calls given method on every existing KMTopLevelWidget. */
  static void forEvery(KForEvery func);

  /** Read configuration. Default implementation is empty. */
  virtual void readConfig(void);

  /** Write configuration. Default implementation is empty. */
  virtual void writeConfig(void);

  /** Closes the widget and the app if no other top level widget
    is opened. Returns TRUE if the widget was closed, otherwise
    FALSE. Call with forceKill==TRUE to delete the widget also. */
    //  virtual bool close(bool forceKill=FALSE);

protected:
   virtual void closeEvent(QCloseEvent*);
};

#endif /*kmtopwidget_h*/
