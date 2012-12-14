/*
  Copyright (c) 2012 Montel Laurent <montel@kde.org>
  
  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.
  
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.
  
  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef IDENTITYEDITVCARDDIALOG_H
#define IDENTITYEDITVCARDDIALOG_H

#include <KDialog>

namespace Akonadi {
class ContactEditor;
}

class IdentityEditVcardDialog : public KDialog
{
  Q_OBJECT
public:
  explicit IdentityEditVcardDialog(QWidget *parent = 0);
  ~IdentityEditVcardDialog();
    /**
   * @brief loadVcard load vcard in a contact editor
   * @param vcardFileName
   */
  void loadVcard(const QString& vcardFileName);
  /**
   * @brief saveVcard
   * @return The file path for current vcard.
   */
  QString saveVcard();
private:
  QString mVcardFileName;
  Akonadi::ContactEditor *mContactEditor;
};

#endif // IDENTITYEDITVCARDDIALOG_H
