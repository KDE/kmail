/* Copyright 2010 Thomas McGuire <mcguire@kde.org>

   Copyright 2011 Laurent Montel <montel@kde.org>

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
#ifndef TAGACTIONMANAGER_H
#define TAGACTIONMANAGER_H

#include "kmail_export.h"
#include "tag.h"
#include <QMap>

namespace Nepomuk2
{
class Resource;
namespace Query
{
class QueryServiceClient;
class Result;
}
}

class KActionCollection;
class KXMLGUIClient;
class KToggleAction;
class QAction;
class QSignalMapper;
class KAction;
namespace Akonadi {
  class Item;
}

namespace KMail {

  class MessageActions;

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


    Q_SIGNALS:

      /**
        * Emitted when one of the tagging actions was triggered. The user of this class
        * should connect to this signal and change the tags of the messages
        */
      void tagActionTriggered( const QString &tagLabel );
      /**
       * Emitted when we want to select more action
       */
      void tagMoreActionClicked();

    private Q_SLOTS:
      void newTagEntries(const QList<Nepomuk2::Query::Result>& results);
      void finishedTagListing();
      void tagsChanged();
      void resourceCreated(const Nepomuk2::Resource&,const QList<QUrl>&);
      void resourceRemoved(const QUrl&,const QList<QUrl>&);
      void propertyChanged(const Nepomuk2::Resource&);
      void newTagActionClicked();

    private:
      void createTagAction( const MailCommon::Tag::Ptr &tag, bool addToMenu );
      void createTagActions();

      QList<QUrl> checkedTags() const;
      void checkTags( const QList<QUrl> &tags );

      KActionCollection *mActionCollection;
      MessageActions *mMessageActions;
      QSignalMapper *mMessageTagToggleMapper;
      KXMLGUIClient *mGUIClient;

      QAction *mSeparatorMoreAction;
      QAction *mSeparatorNewTagAction;
      KAction *mMoreAction;
      KAction *mNewTagAction;
      // Maps the resource URI or a tag to the action of a tag.
      // Contains all existing tags
      QMap<QString,KToggleAction*> mTagActions;

      // The actions of all tags that are in the toolbar
      QList<QAction*> mToolbarActions;

      // Cache of the tags to avoid expensive Nepomuk queries
      QList<MailCommon::Tag::Ptr> mTags;

      Nepomuk2::Query::QueryServiceClient *mTagQueryClient;

      // Uri of a newly created tag
      QString mNewTagUri;
 };
}

#endif
