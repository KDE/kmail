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
#ifndef kmfiltermgr_h
#define kmfiltermgr_h

#include "kmfilteraction.h" // for KMFilterAction::ReturnCode
#include "kmfolder.h"

#include <qptrlist.h>
#include <qguardedptr.h>
#include <qobject.h>

class KMFilter;
class KMFilterDlg;

class KMFilterMgr: public QObject, public QPtrList<KMFilter>
{
  Q_OBJECT

public:
  KMFilterMgr(bool popFilter = false);
  virtual ~KMFilterMgr();

  enum FilterSet { NoSet = 0x0, Inbound = 0x1, Outbound = 0x2, Explicit = 0x4,
		   All = Inbound|Outbound|Explicit };

  /** Reload filter rules from config file. */
  void readConfig(void);

  /** Store filter rules in config file. */
  void writeConfig(bool withSync=TRUE);

  /** Open an edit dialog. If checkForEmptyFilterList is true, an empty filter
      is created to improve the visibility of the dialog in case no filter
      has been defined so far. */
  void openDialog( QWidget *parent, bool checkForEmptyFilterList = true );

  /** Open an edit dialog, create a new filter and preset the first
      rule with "field equals value" */
  void createFilter( const QCString & field, const QString & value );

  bool beginFiltering(KMMsgBase *msgBase) const;
  int moveMessage(KMMessage *msg) const;
  void endFiltering(KMMsgBase *msgBase) const;

  /** Check for existing filters with the &p name and extend the
      "name" to "name (i)" until no match is found for i=1..n */
  const QString createUniqueName( const QString & name );

  /** Append the list of filters to the current list of filters and
      write everything back into the configuration.*/
  void appendFilters( const QPtrList<KMFilter> filters );

  /** Process given message by applying the filter rules one by
      one. You can select which set of filters (incoming or outgoing)
      should be used.

      @param msg The message to process.
      @param aSet Select the filter set to use.
      @return 2 if a critical error occurred (eg out of disk space)
      1 if the caller is still owner of the message and
      0 otherwise. If the caller does not any longer own the message
      he *must* not delete the message or do similar stupid things. ;-)
  */
  int process( KMMessage * msg, FilterSet aSet=Inbound );

  /** For ad-hoc filters. Applies @p filter to @p msg. Return codes
      are as with the above method. */
  int process( KMMessage * msg, const KMFilter * filter );

  void cleanup();
  /** Increment the reference count for the filter manager.
      Call this method before processing messages with process() */
  void ref();
  /** Decrement the reference count for the filter manager.
      Call this method after processing messages with process().
      Shall be called after all messages are processed.
      If the reference count is zero then this method closes all folders
      that have been temporarily opened with tempOpenFolder(). */
  void deref(bool force = false);

  /** Open given folder and mark it as temporarily open. The folder
    will be closed upon next call of cleanip(). This method is
    usually only called from within filter actions during process().
    Returns returncode from KMFolder::open() call. */
  int tempOpenFolder(KMFolder* aFolder);

  /** Called at the beginning of an filter list update. Currently a
      no-op */
  void beginUpdate() {}

  /** Called at the end of an filter list update. */
  void endUpdate();

  /** Output all rules to stdout */
#ifndef NDEBUG
  void dump();
#endif

  /** Called from the folder manager when a folder is removed.
    Tests if the folder aFolder is used in any action. Changes
    to aNewFolder folder in this case. Returns TRUE if a change
    occurred. */
  bool folderRemoved(KMFolder* aFolder, KMFolder* aNewFolder);

  /** Called from the folder manager when a new folder has been
      created. Forwards this to the filter dialog if that is open. */
  void folderCreated(KMFolder*) {}

  /** Set the global option 'Show Download Later Messages' */
  void setShowLaterMsgs( bool show ) {
    mShowLater = show;
  }

  /** Get the global option 'Show Download Later Messages' */
  bool showLaterMsgs() const {
    return mShowLater;
  }
public slots:
  void slotFolderRemoved( KMFolder *aFolder );

signals:
  void filterListUpdated();

private:
  int processPop( KMMessage * msg ) const;

  QGuardedPtr<KMFilterDlg> mEditDialog;
  QPtrList<KMFolder> mOpenFolders;
  bool bPopFilter;
  bool mShowLater;
  int mRefCount;
};

#endif /*kmfiltermgr_h*/
