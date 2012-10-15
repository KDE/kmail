/* Copyright 2010 Thomas McGuire <mcguire@kde.org>
   Copyright 2012 Laurent Montel <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "tag.h"

#include "messagetag.h"

#include <soprano/nao.h>
#include <Nepomuk2/Tag>
#include <Nepomuk2/Variant>
#include <QDebug>
using namespace KMail;

Tag::Ptr Tag::fromNepomuk( const Nepomuk2::Tag& nepomukTag )
{
  Tag::Ptr tag( new Tag() );
  tag->tagName = nepomukTag.label();

  tag->iconName = nepomukTag.genericIcon();
  if ( tag->iconName.isEmpty() )
    tag->iconName = "mail-tagged";

  tag->nepomukResourceUri = nepomukTag.uri();

  if ( nepomukTag.hasProperty( Vocabulary::MessageTag::textColor() ) ) {
    const QString name = nepomukTag.property( Vocabulary::MessageTag::textColor() ).toString();
    tag->textColor = QColor( name );
  }

  if ( nepomukTag.hasProperty( Vocabulary::MessageTag::backgroundColor() ) ) {
    const QString name = nepomukTag.property( Vocabulary::MessageTag::backgroundColor() ).toString();
    tag->backgroundColor = QColor( name );
  }

  if ( nepomukTag.hasProperty( Vocabulary::MessageTag::font() ) ) {
    const QString fontString = nepomukTag.property( Vocabulary::MessageTag::font() ).toString();
    QFont font;
    font.fromString( fontString );
    tag->textFont = font;
  }

  if ( nepomukTag.hasProperty( Vocabulary::MessageTag::priority() ) ) {
    tag->priority = nepomukTag.property( Vocabulary::MessageTag::priority() ).toInt();
  }
  else
    tag->priority = -1;

  if ( nepomukTag.hasProperty( Vocabulary::MessageTag::shortcut() ) ) {
    tag->shortcut = KShortcut( nepomukTag.property( Vocabulary::MessageTag::shortcut() ).toString() );
  }

  if ( nepomukTag.hasProperty( Vocabulary::MessageTag::inToolbar() ) ) {
    tag->inToolbar = nepomukTag.property( Vocabulary::MessageTag::inToolbar() ).toBool();
  }
  else
    tag->inToolbar = false;

  return tag;
}

void Tag::saveToNepomuk( SaveFlags saveFlags ) const
{
  Nepomuk2::Tag nepomukTag( nepomukResourceUri );

  nepomukTag.setLabel( tagName );

  Nepomuk2::Resource symbol( QUrl(), Soprano::Vocabulary::NAO::FreeDesktopIcon() );
  symbol.setProperty( Soprano::Vocabulary::NAO::iconName(), iconName );

  nepomukTag.setProperty( Soprano::Vocabulary::NAO::hasSymbol(), symbol );

  nepomukTag.setProperty( Vocabulary::MessageTag::priority(), priority );
  nepomukTag.setProperty( Vocabulary::MessageTag::inToolbar(), inToolbar );
  nepomukTag.setProperty( Vocabulary::MessageTag::shortcut(), shortcut.toString() );

  if ( textColor.isValid() && saveFlags & TextColor )
    nepomukTag.setProperty( Vocabulary::MessageTag::textColor(), textColor.name() );
  else
    nepomukTag.removeProperty( Vocabulary::MessageTag::textColor() );

  if ( backgroundColor.isValid() && saveFlags & BackgroundColor )
    nepomukTag.setProperty( Vocabulary::MessageTag::backgroundColor(), backgroundColor.name() );
  else
    nepomukTag.removeProperty( Vocabulary::MessageTag::backgroundColor() );

  if ( saveFlags & Font )
    nepomukTag.setProperty( Vocabulary::MessageTag::font(), textFont.toString() );
  else
    nepomukTag.removeProperty( Vocabulary::MessageTag::font() );
}

bool Tag::compare( Tag::Ptr &tag1, Tag::Ptr &tag2 )
{
  if ( tag1->priority < tag2->priority )
    return true;
  else if (tag1->priority == tag2->priority)
    return ( tag1->tagName < tag2->tagName );
  else
    return false;
}

bool Tag::compareName( Tag::Ptr &tag1, Tag::Ptr &tag2 )
{
  return ( tag1->tagName < tag2->tagName );
}

bool Tag::operator==( const Tag &other ) const
{
  return tagName == other.tagName &&
         textColor == other.textColor &&
         backgroundColor == other.backgroundColor &&
         textFont == other.textFont &&
         iconName == other.iconName &&
         inToolbar == other.inToolbar &&
         shortcut.toString() == other.shortcut.toString() &&
         priority == other.priority &&
         nepomukResourceUri == other.nepomukResourceUri;
}

bool Tag::operator!=( const Tag &other ) const
{
  return !( *this == other );
}
