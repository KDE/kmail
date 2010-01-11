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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifndef kmfiltermgr_h
#define kmfiltermgr_h

#include "kmfilteraction.h" // for KMFilterAction::ReturnCode
#include <akonadi/collection.h>

namespace Akonadi {
  class Item;
}
namespace KMime {
  class Message;
}

class KMFolder;
class KMFilter;
class KMFilterDlg;

class KMFilterMgr: public QObject
{
  Q_OBJECT

public:
  KMFilterMgr(bool popFilter = false);
  virtual ~KMFilterMgr();

  /** Clears the list of filters and deletes them. */
  void clear();

  enum FilterSet { NoSet = 0x0, Inbound = 0x1, Outbound = 0x2, Explicit = 0x4,
       BeforeOutbound = 0x8, All = Inbound|BeforeOutbound|Outbound|Explicit };

  /** Reload filter rules from config file. */
  void readConfig(void);

  /** Store filter rules in config file. */
  void writeConfig(bool withSync=true);

  /** Open an edit dialog. If checkForEmptyFilterList is true, an empty filter
      is created to improve the visibility of the dialog in case no filter
      has been defined so far. */
  void openDialog( QWidget *parent, bool checkForEmptyFilterList = true );

  /** Open an edit dialog, create a new filter and preset the first
      rule with "field equals value" */
  void createFilter( const QByteArray & field, const QString & value );

  bool beginFiltering( const Akonadi::Item &item ) const;
  void endFiltering( const Akonadi::Item &item ) const;

  /**
   * Returns whether at least one filter applies to this account,
   * which means that mail must be downloaded in order to be filtered,
   * for example;
   * */
  bool atLeastOneFilterAppliesTo( const QString& accountID ) const;

  /**
   * Returns whether at least one incoming filter applies to this account,
   * which means that mail must be downloaded in order to be filtered,
   * for example;
   * */
  bool atLeastOneIncomingFilterAppliesTo( const QString & accountID ) const;

  /** Returns whether at least one filter targets a folder on an
   * online IMAP account.
   * */
  bool atLeastOneOnlineImapFolderTarget();

  /** Check for existing filters with the &p name and extend the
      "name" to "name (i)" until no match is found for i=1..n */
  const QString createUniqueName( const QString & name );

  /** Append the list of filters to the current list of filters and
      write everything back into the configuration. The filter manager
      takes ownership of the filters in the list. */
  void appendFilters( const QList<KMFilter*> &filters,
                      bool replaceIfNameExists = false );

  /** Replace the list of filters under control of the filter manager.
   * The manager takes ownershipt of the filters. */
  void setFilters( const QList<KMFilter*> &filters );

  /** @return the list of filters managed by this object */
  const QList<KMFilter*> & filters() const { return mFilters; }

  /** Process given message by applying the filter rules one by
      one. You can select which set of filters (incoming or outgoing)
      should be used.

      @param msg The message to process.
      @param aSet Select the filter set to use.
      @param account true if an account id is specified else false
      @param accountId The id of the KMAccount that the message was
             retrieved from
      @return 2 if a critical error occurred (eg out of disk space)
      1 if the caller is still owner of the message and
      0 otherwise. If the caller does not any longer own the message
      he *must* not delete the message or do similar stupid things. ;-)
  */
  int process( const Akonadi::Item &item, FilterSet aSet = Inbound,
	       bool account = false, const QString & accountId = QString() );

  /** For ad-hoc filters. Applies @p filter to message @p item.
      Return codes are as with the above method. */
  int process( const Akonadi::Item &item, const KMFilter * filter );

  void cleanup();

  /** Called at the beginning of an filter list update. Currently a
      no-op */
  void beginUpdate() {}

  /** Called at the end of an filter list update. */
  void endUpdate();

  /** Output all rules to stdout */
#ifndef NDEBUG
  void dump() const;
#endif

  /** Called from the folder manager when a folder is removed.
    Tests if the folder aFolder is used in any action. Changes
    to aNewFolder folder in this case. Returns true if a change
    occurred. */
  bool folderRemoved(const Akonadi::Collection& aFolder, const Akonadi::Collection& aNewFolder);

  /** Set the global option 'Show Download Later Messages' */
  void setShowLaterMsgs( bool show ) {
    mShowLater = show;
  }

  /** Get the global option 'Show Download Later Messages' */
  bool showLaterMsgs() const {
    return mShowLater;
  }
public slots:
  void slotFolderRemoved( const Akonadi::Collection &aFolder );

signals:
  void filterListUpdated();

private:
  int processPop( const Akonadi::Item &item ) const;
  /** Find out if a message matches the filter criteria */
  bool isMatching( const Akonadi::Item &item, const KMFilter * filter );

  QPointer<KMFilterDlg> mEditDialog;
  QList<KMFilter *> mFilters;
  bool bPopFilter;
  bool mShowLater;
  bool mDirtyBufferedFolderTarget;
  bool mBufferedFolderTarget;
};

#endif /*kmfiltermgr_h*/
