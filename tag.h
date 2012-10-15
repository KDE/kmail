/* Copyright 2010 Thomas McGuire <mcguire@kde.org>

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
#ifndef TAG_H
#define TAG_H

#include <KShortcut>

#include <QColor>
#include <QFont>
#include <QSharedPointer>
#include <QUrl>

namespace Nepomuk2 {
  class Tag;
}

namespace KMail {

  // Our own copy of the tag data normally attached to a Nepomuk2::Tag.
  // Useful in the config dialog, because the user might cancel his changes,
  // in which case we don't write them back to the Nepomuk2::Tag.
  // Also used as a convenience class in the TagActionManager.
  class Tag
  {
    Q_GADGET
    public:

      typedef QSharedPointer<Tag> Ptr;
      enum SaveFlag {
        TextColor = 1,
        BackgroundColor = 1 << 1,
        Font = 1 << 2
      };
      typedef QFlags<SaveFlag> SaveFlags;

      // Returns true if two tags are equal
      bool operator==( const Tag &other ) const;

      bool operator!=( const Tag &other ) const;

      // Load a tag from a Nepomuk tag
      static Ptr fromNepomuk( const Nepomuk2::Tag& nepomukTag );

      // Save this tag to Nepomuk the corresponding Nepomuk tag
      void saveToNepomuk( SaveFlags saveFlags ) const;

      // Compare, based on priority
      static bool compare( Ptr &tag1, Ptr &tag2 );
      // Compare, based on name
      static bool compareName( Ptr &tag1, Ptr &tag2 );

      QString tagName;
      QColor textColor;
      QColor backgroundColor;
      QFont textFont;
      QString iconName;
      QUrl nepomukResourceUri;
      KShortcut shortcut;
      bool inToolbar;
      bool tagStatus;

      // Priority, i.e. sort order of the tag. Only used when loading the tag, when saving
      // the priority is set to the position in the list widget
      int priority;
  };
  Q_DECLARE_OPERATORS_FOR_FLAGS(Tag::SaveFlags)

}

#endif
