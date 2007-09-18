/**
 * kmeditor.h
 *
 * Copyright (C)  2007  Laurent Montel <montel@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef KMCOMPOSEREDITOR_H
#define KMCOMPOSEREDITOR_H

#include <kmeditor.h>
using namespace KPIM;

class KMComposeWin;

class KMComposerEditor : public KMeditor
{
    Q_OBJECT

  public:
    /**
     * Constructs a KMComposerEditor object.
     */
    explicit KMComposerEditor( KMComposeWin *win,QWidget *parent = 0 );

    ~KMComposerEditor();

    virtual QString quotePrefixName() const;
    virtual QString smartQuote( const QString & msg );
  private:
     KMComposeWin *m_composerWin;
  protected:
     void dropEvent( QDropEvent *e );
  signals:
     void attachPNGImageData( const QByteArray &image );
};

#endif

