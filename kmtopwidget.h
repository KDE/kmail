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
 * to the Free Software Foundation, Inc, 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */
#ifndef kmtopwidget_h
#define kmtopwidget_h

#include <kmainwindow.h>

class QCloseEvent;

/** Top level window that offers methods to be called on every
 * existing top level window.
 */
class KMTopLevelWidget: public KMainWindow
{
  Q_OBJECT

public:
  KMTopLevelWidget(const char *name = 0);
  ~KMTopLevelWidget();

  /** Read configuration. Default implementation is empty. */
  virtual void readConfig(void);

  /** Write configuration. Default implementation is empty. */
  virtual void writeConfig(void);

protected:
  virtual void closeEvent(QCloseEvent*);
};

#endif /*kmtopwidget_h*/
