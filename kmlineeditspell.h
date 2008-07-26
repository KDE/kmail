/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>

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

#ifndef __KMAIL_KMLINEEDITSPELL_H__
#define __KMAIL_KMLINEEDITSPELL_H__

#include <libkdepim/addresseelineedit.h>
#include <QKeyEvent>
#include <QMenu>
#include <QDropEvent>

class KMLineEdit : public KPIM::AddresseeLineEdit
{
  Q_OBJECT

  public:
    explicit KMLineEdit( bool useCompletion, QWidget *parent = 0,
                         const char *name = 0 );

  signals:
    void focusUp();
    void focusDown();

  protected:

    // Inherited. Always called by the parent when this widget is created.
    virtual void loadContacts();

    virtual void keyPressEvent(QKeyEvent*);

    virtual void contextMenuEvent( QContextMenuEvent*e );

  private slots:
    void editRecentAddresses();

  private:
    void dropEvent( QDropEvent *event );
    void insertEmails( const QStringList & emails );
};


class KMLineEditSpell : public KMLineEdit
{
  Q_OBJECT

  public:
    explicit KMLineEditSpell( bool useCompletion, QWidget *parent = 0,
                              const char *name = 0 );
    void highLightWord( unsigned int length, unsigned int pos );
    void spellCheckDone( const QString &s );
    void spellCheckerMisspelling( const QString &text, const QStringList &, unsigned int pos);
    void spellCheckerCorrected( const QString &old, const QString &corr, unsigned int pos);

  signals:
    void subjectTextSpellChecked();
};

#endif // __KMAIL_KMLINEEDITSPELL_H__
