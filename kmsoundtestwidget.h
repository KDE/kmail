/*
  kmfawidgets.h - KMFilterAction parameter widgets
  Copyright (c) 2001 Marc Mutz <mutz@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef _KMSOUND_TEST_WIDGET_H
#define _KMSOUND_TEST_WIDGET_H

#include <KLineEdit>

#include <QString>

class QPushButton;
class KUrlRequester;

class KMSoundTestWidget : public QWidget
{
  Q_OBJECT
public:
  explicit KMSoundTestWidget( QWidget * parent, const char * name=0 );
  ~KMSoundTestWidget();
  QString url() const;
  void setUrl( const QString & url );
  void clear();
signals:
  void testPressed();
protected slots:
  void playSound();
  void openSoundDialog( KUrlRequester * );
  void slotUrlChanged( const QString & );

private:
  KUrlRequester *m_urlRequester;
  QPushButton *m_playButton;
};

#endif
