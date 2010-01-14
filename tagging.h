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
#ifndef TAGGING_H
#define TAGGING_H

#include "kmail_export.h"

#include <Nepomuk/Tag>
#include <Soprano/Statement>
#include <Soprano/Util/SignalCacheModel>

#include <KShortcut>

#include <QString>
#include <QColor>
#include <QFont>
#include <QUrl>
#include <QSharedPointer>
#include <QScopedPointer>
#include <QFlags>
#include <QMap>

class KAction;
class KActionCollection;
class KToggleAction;
class KXMLGUIClient;
class QSignalMapper;
class QAction;

namespace Akonadi {
  class Item;
}

namespace KMail {

  class MessageActions;

  // Our own copy of the tag data normally attached to a Nepomuk::Tag.
  // Useful in the config dialog, because the user might cancel his changes,
  // in which case we don't write them back to the Nepomuk::Tag.
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

      // Load a tag from a Nepomuk tag
      static Ptr fromNepomuk( const Nepomuk::Tag& nepomukTag );

      // Save this tag to Nepomuk the corresponding Nepomuk tag
      void saveToNepomuk( SaveFlags saveFlags ) const;

      // Compare, based on priority
      static bool compare( Ptr &tag1, Ptr &tag2 );

      QString tagName;
      QColor textColor;
      QColor backgroundColor;
      QFont textFont;
      QString iconName;
      QUrl nepomukResourceUri;
      KShortcut shortcut;
      bool inToolbar;

      // Priority, i.e. sort order of the tag. Only used when loading the tag, when saving
      // the priority is set to the position in the list widget
      int priority;
  };
  Q_DECLARE_OPERATORS_FOR_FLAGS(Tag::SaveFlags)

  /**
    * Creates actions related to the existing Nepomuk tags and plugs them into the GUI.
    *
    * The tag manager reads all tags from Nepomuk and adds each to the action collection
    * and to the message status menu.
    * For tags that should be in the toolbar, it plugs the action list
    * toolbar_messagetag_actions.
    *
    * The actions are automatically updated when a Nepomuk tag changes.
    */
  class KMAIL_EXPORT TagActionManager : public QObject
  {
    Q_OBJECT
    public:

      /**
        * Does not take ownership of the action collection, the GUI client or the message actions.
        * Does not yet create the actions.
        *
        * @param actionCollection: Each tag action is added here
        * @param messageActions: Each action is added to the message status menu
        * @param guiClient: The action list with the toolbar action is plugged here
        */
      TagActionManager( QObject *parent, KActionCollection *actionCollection,
                        MessageActions *messageActions, KXMLGUIClient *guiClient );

      ~TagActionManager();

      /**
        * Removes all actions from the GUI again
        */
      void clearActions();

      /**
        * Creates and plugs all tag actions
        */
      void createActions();

      /**
        * Updates the state of the toggle actions of all tags.
        * The state of the action depends on the number of selected messages, for example
        * all actions are disabled when no message is selected.
        *
        * @param selectedItem if exactly one item is selected, it should be passed here
        */
      void updateActionStates( int numberOfSelectedMessages,
                               const Akonadi::Item &selectedItem );

      /**
        * Triggers an update of all instances of TagActionManager.
        */
      static void triggerUpdate();


    Q_SIGNALS:

      /**
        * Emitted when one of the tagging actions was triggered. The user of this class
        * should connect to this signal and change the tags of the messages
        */
      void tagActionTriggered( const QString &tagLabel );

    private Q_SLOTS:

      void statementChanged( Soprano::Statement statement );

    private:

      KActionCollection *mActionCollection;
      MessageActions *mMessageActions;
      QSignalMapper *mMessageTagToggleMapper;
      KXMLGUIClient *mGUIClient;

      // Maps the resource URI or a tag to the action of a tag.
      // Contains all existing tags
      QMap<QString,KToggleAction*> mTagActions;

      // The actions of all tags that are in the toolbar
      QList<QAction*> mToolbarActions;

      // Needed so we can listen to Nepomuk Tag changes
      QScopedPointer<Soprano::Util::SignalCacheModel> mSopranoModel;

      static QList<TagActionManager*> mInstances;
  };

}
#endif
