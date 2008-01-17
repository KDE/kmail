/*  -*- mode: C++; c-file-style: "gnu" -*-
    dictionarycombobox.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2003 Ingo Kloecker <kloecker@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "dictionarycombobox.h"

#include <kdebug.h>
#include <sonnet/speller.h>

#include <QStringList>

namespace KMail {

  DictionaryComboBox::DictionaryComboBox( QWidget * parent )
    : QComboBox( parent ),
      mDefaultDictionary( 0 )
  {
    reloadCombo();
    connect( this, SIGNAL( activated( int ) ),
             this, SLOT( slotDictionaryChanged( int ) ) );
    /*    connect( this, SIGNAL( dictionaryChanged( int ) ),
             mSpellConfig, SLOT( sSetDictionary( int ) ) );
    */
  }

  DictionaryComboBox::~DictionaryComboBox()
  {
  }

  QString DictionaryComboBox::currentDictionaryName() const
  {
    return currentText();
  }

  QString DictionaryComboBox::currentDictionary() const
  {
    if ( mDictionaries.empty() )
      return "<default>";
    QString dict = mDictionaries[ currentIndex() ];
    if ( dict.isEmpty() )
      return "<default>";
    else
      return dict;
  }

  void DictionaryComboBox::setCurrentByDictionaryName( const QString & name )
  {
    if ( name.isEmpty() )
      return;

    for ( int i = 0; i < count(); ++i ) {
      if ( itemText( i ) == name ) {
        if ( i != currentIndex() ) {
          setCurrentIndex( i );
          slotDictionaryChanged( i );
        }
        return;
      }
    }
  }

  void DictionaryComboBox::setCurrentByDictionary( const QString & dictionary )
  {
    if ( !dictionary.isEmpty() ) {
      // first handle the special case of the default dictionary
      if ( dictionary == "<default>" ) {
        if ( 0 != currentIndex() ) {
          setCurrentIndex( 0 );
          slotDictionaryChanged( 0 );
        }
        return;
      }

      int i = 0;
      for ( QStringList::ConstIterator it = mDictionaries.begin();
            it != mDictionaries.end();
            ++it, ++i ) {
        if ( *it == dictionary ) {
          if ( i != currentIndex() ) {
            setCurrentIndex( i );
            slotDictionaryChanged( i );
          }
          return;
        }
      }
    }

    // If dictionary is empty or doesn't exist fall back to the global default
    if ( mDefaultDictionary != currentIndex() ) {
      setCurrentIndex( mDefaultDictionary );
      slotDictionaryChanged( mDefaultDictionary );
    }
  }
  /*
  K3SpellConfig* DictionaryComboBox::spellConfig() const
  {
    return mSpellConfig;
  }
  */

  void DictionaryComboBox::reloadCombo()
  {
    mspeller = new Sonnet::Speller();
    insertStringList( mspeller->availableLanguageNames() );

    /*    delete mSpellConfig;
    mSpellConfig = new K3SpellConfig( 0, 0, false );
    mSpellConfig->fillDicts( this, &mDictionaries );
    mDefaultDictionary = currentIndex();
    */
  }

  void DictionaryComboBox::slotDictionaryChanged( int idx )
  {
    kDebug( 5006 ) <<"DictionaryComboBox::slotDictionaryChanged(" << idx << ")";
    if( !mDictionaries.isEmpty())
    	emit dictionaryChanged( mDictionaries[idx] );
    emit dictionaryChanged( idx );
  }

} // namespace KMail

#include "dictionarycombobox.moc"
