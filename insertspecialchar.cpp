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

#include "insertspecialchar.h"
#include <KCharSelect>
#include <KLocale>
#include <QHBoxLayout>

InsertSpecialChar::InsertSpecialChar(QWidget *parent)
  :KDialog(parent)
{
  setCaption( i18n("Insert Special Characters") );
  setButtons( Ok|Cancel|User1 );
  setButtonText(KDialog::User1,i18n("Insert"));
  QWidget *page = new QWidget( this );
  setMainWidget( page );
  QHBoxLayout *lay = new QHBoxLayout(page);
  mCharSelect = new KCharSelect(this,KCharSelect::CharacterTable|KCharSelect::BlockCombos);
  connect(mCharSelect,SIGNAL(charSelected(QChar)),this,SIGNAL(charSelected(QChar)));
  lay->addWidget(mCharSelect);
  connect(this,SIGNAL(user1Clicked()),SLOT(slotInsertChar()));
}

InsertSpecialChar::~InsertSpecialChar()
{
}


void InsertSpecialChar::slotInsertChar()
{
  Q_EMIT charSelected(mCharSelect->currentChar());
}

#include "insertspecialchar.moc"
