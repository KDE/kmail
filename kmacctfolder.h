/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
#ifndef kmacctfolder_h
#define kmacctfolder_h

#include "kmfolder.h"

class KMAccount;

/** Simple wrapper class that contains the kmail account handling
 * stuff that is usually not required outside kmail.
 *
 * WARNING: do not add virtual methods in this class. This class is
 * used as a typecast for KMFolder only !!
 * @author Stefan Taferner
 */
class KMAcctFolder: public KMFolder
{
public:
  /** Returns first account or 0 if no account is associated with this
      folder */
  KMAccount* account(void);

  /** Returns next account or 0 if at the end of the list */
  KMAccount* nextAccount(void);

  /** Add given account to the list */
  void addAccount(KMAccount*);

  /** Remove given account from the list */
  void removeAccount(KMAccount*);

  /** Clear list of accounts */
  void clearAccountList(void);

private:
  KMAcctFolder( KMFolderDir* parent, const QString& name,
                KMFolderType aFolderType );
};


#endif /*kmacctfolder_h*/
