/*  -*- mode: C++; c-file-style: "gnu" -*-
 *
 *  This file is part of KMail, the KDE mail client.
 *  Copyright (c) 2003 Carsten Burghardt <burghardt@kde.org>
 *
 *  KMail is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License, version 2, as
 *  published by the Free Software Foundation.
 *
 *  KMail is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */

#ifndef bodyiterator_h
#define bodyiterator_h

#include <qptrlist.h>
#include <qstringlist.h>

class KMMessagePart;

namespace KMail {

  class AttachmentStrategy;

  class BodyVisitor
  {
    public:
      BodyVisitor();
      virtual ~BodyVisitor();

      void visit( KMMessagePart * part );
      void visit( QPtrList<KMMessagePart> & list );

      virtual QPtrList<KMMessagePart> partsToLoad() = 0;

    protected:
      QPtrList<KMMessagePart> mParts;
      QStringList mBasicList;
  };

  class BodyVisitorFactory
  {
    public:
      static BodyVisitor* getVisitor( const AttachmentStrategy* strategy );
  };

  class BodyVisitorSmart: public BodyVisitor
  {
    public:
      BodyVisitorSmart();

      QPtrList<KMMessagePart> partsToLoad();
  };
  
  class BodyVisitorInline: public BodyVisitor
  {
    public:
      BodyVisitorInline();

      QPtrList<KMMessagePart> partsToLoad();
  };
  
  class BodyVisitorHidden: public BodyVisitor
  {
    public:
      BodyVisitorHidden();

      QPtrList<KMMessagePart> partsToLoad();
  };
  
};

#endif
