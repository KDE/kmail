/*******************************************************************************
**
** Filename   : newfolderdialog.h
** Created on : 30 January, 2005
** Copyright  : (c) 2005 Till Adam
** Email      : adam@kde.org
**
*******************************************************************************/

/*******************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
**   In addition, as a special exception, the copyright holders give
**   permission to link the code of this program with any edition of
**   the Qt library by Trolltech AS, Norway (or with modified versions
**   of Qt that use the same license as Qt), and distribute linked
**   combinations including the two.  You must obey the GNU General
**   Public License in all respects for all of the code used other than
**   Qt.  If you modify this file, you may extend this exception to
**   your version of the file, but you are not obligated to do so.  If
**   you do not wish to do so, delete this exception statement from
**   your version.
*******************************************************************************/

#ifndef NEW_FOLDER_DIALOG_H
#define NEW_FOLDER_DIALOG_H

#include <qvariant.h>
#include <qdialog.h>
#include <kdialogbase.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
class QLabel;
class QLineEdit;
class QComboBox;
class KMFolder;

namespace KMail {

class NewFolderDialog : public KDialogBase
{
  Q_OBJECT

  public:
    NewFolderDialog( QWidget* parent = 0, KMFolder *folder = 0 );
    ~NewFolderDialog() {};

    QLabel* mNameLabel;
    QLineEdit* mNameLineEdit;
    QLabel* mMailboxFormatLabel;
    QComboBox* mFormatComboBox;
    QLabel* mContentsLabel;
    QComboBox* mContentsComboBox;

  protected:
    QVBoxLayout* mTopLevelLayout;
    QHBoxLayout* mNameHBox;
    QHBoxLayout* mFormatHBox;
    QHBoxLayout* mContentsHBox;
  protected slots:
    void slotOk();

  private:
    KMFolder* mFolder;
};

} // namespace
#endif // NEW_FOLDER_DIALOG_H
