/* -*- Mode: C -*-

  $Id$

  CRYPTPLUG - an independent cryptography plug-in
  API. CryptPlugWrapperList holds any number of crypto plug-ins.

  Copyright (C) 2001 by Klarälvdalens Datakonsult AB

  CRYPTPLUG is free software; you can redistribute it and/or modify
  it under the terms of GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  CRYPTPLUG is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
*/

#ifndef __CRYPTPLUGWRAPPERLIST_H__
#define __CRYPTPLUGWRAPPERLIST_H__

#include "cryptplugwrapper.h"

#include <qptrlist.h>

class KConfig;

class CryptPlugWrapperList : public QPtrList<CryptPlugWrapper>
{
public:
    CryptPlugWrapper* active() const;
    void loadFromConfig( KConfig* config );
};



#endif
