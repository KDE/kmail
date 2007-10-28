/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2006 Dmitry Morozhnikov
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */


#ifndef TEMPLATESCONFIGURATION_H
#define TEMPLATESCONFIGURATION_H

#include "ui_templatesconfiguration_base.h"
#include "templatesinsertcommand.h"

class TemplatesConfiguration : public QWidget, Ui::TemplatesConfigurationBase
{
  Q_OBJECT

  public:

    explicit TemplatesConfiguration( QWidget *parent = 0, const char *name = 0 );

    void loadFromGlobal();
    void saveToGlobal();
    void loadFromIdentity( uint id );
    void saveToIdentity( uint id );
    void loadFromFolder( const QString &id, uint identity = 0 );
    void saveToFolder( const QString &id );

    /** Do import settings from 'Phrases' configuration. */
    void loadFromPhrases();

    /** Do import 'Phrases' settings and store them as global templates. */
    static void importFromPhrases();

    /** Convert 'Phrases'-like placeholders into 'Templates' compatible. */
    static QString convertPhrases( const QString &str );

    static QString defaultNewMessage();
    static QString defaultReply();
    static QString defaultReplyAll();
    static QString defaultForward();
    static QString defaultQuoteString();

    QLabel *helpLabel() const { return mHelp; }

  public slots:
    void slotInsertCommand( const QString &cmd, int adjustCursor = 0 );
    void slotTextChanged();

  signals:
    void changed();

  protected:
    QString strOrBlank( const QString &str );
    QString mHelpString;

  private slots:
    void slotHelpLinkClicked( const QString& );
};

#endif // TEMPLATESCONFIGURATION_H
