/*
    kmgroupware.h

    This file is part of KMail.

    Copyright (c) 2003 Bo Thorsen <bo@klaralvdalens-datakonsult.se>
    Copyright (c) 2002 Karl-Heinz Zimmer <khz@klaralvdalens-datakonsult.se>
    Copyright (c) 2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef KMGROUPWARE_H
#define KMGROUPWARE_H

#include "kmfoldertype.h"
#include <kfoldertree.h>
#include <qguardedptr.h>

class QSplitter;
class QDateTime;

class DCOPClient;

class KMFolder;
class KMAccount;
class KMMainWin;
class KMMessage;
class KMHeaders;
class KMReaderWin;
class KMMimePartTree;
class KMMsgBase;
class KURL;

namespace KParts {
  class ReadOnlyPart;
};


class KMGroupware : public QObject
{
  Q_OBJECT

public:
  KMGroupware( QObject* parent = 0, const char* name = 0 );
  virtual ~KMGroupware();

signals:
  /** Make the IMAP resource re-read all of the given type */
  void signalRefresh( const QString& type);

private slots:
  // internal slots for new interface
  void slotRefreshCalendar();
  void slotRefreshTasks();

  ////////////////////////////////////////////////////////////////

public:
  bool folderSelected( KMFolder* folder );
  bool checkFolders() const;

  void setupKMReaderWin(KMReaderWin* reader);
  void setMimePartTree(KMMimePartTree* mimePartTree);
  void createKOrgPart(QWidget* parent);
  void reparent(QSplitter* panner);
  void moveToLast();
  void setupActions(); // Not const since it emits a signal
  void enableActions(bool on) const;
  void processVCalRequest( const QCString& receiver, const QString& vCalIn,
                           QString& choice );
  void processVCalReply( const QCString& sender, const QString& vCalIn,
			 const QString& choice );

  /* (Re-)Read configuration file */
  void readConfig();

  bool isEnabled() const { return mUseGroupware; }

  bool hidingMimePartTree(){ return mGroupwareIsHidingMimePartTree; }

  // retrieve matching body part (either text/vCal (or vCard) or application/ms-tnef)
  // and decode it
  // returns a readable vPart in *s or in *sc or in both
  // (please make sure to set s or sc to o if you don't want ot use it)
  // note: Additionally the number of the update counter (if any was found) is
  //       returned in aUpdateCounter, this applies only to TNEF data - in the
  //       iCal standard (RfC2445,2446) there is no update counter.
  static bool vPartFoundAndDecoded( KMMessage* msg, QString& s );

  enum DefaultUpdateCounterValue { NoUpdateCounter=-1 };
  // functions to be called by KMReaderWin for 'print formatting'
  static bool vPartToHTML( int aUpdateCounter, const QString& vCal, QString fname,
                           bool useGroupware, QString& prefix, QString& postfix );
  static bool msTNEFToVPart( const QByteArray& tnef, QString& aVPart );
  static bool msTNEFToHTML( KMReaderWin* reader, QString& vPart, QString fname, bool useGroupware,
                            QString& prefix, QString& postfix );

  // function to be called by KMReaderWin for analyzing of clicked URL
  static bool foundGroupwareLink( const QString aUrl,
                                  QString& gwType,
                                  QString& gwAction,
                                  QString& gwAction2,
                                  QString& gwData );

  /** KMReaderWin calls this with an URL. Return true if a groupware url was
      handled. */
  virtual bool handleLink( const KURL &aUrl, KMMessage* msg );

  /** These methods are called by KMKernel's DCOP functions. */
  virtual void requestAddresses( QString );
  virtual bool storeAddresses(QString, QStringList);

  // automatic resource handling
  bool incomingResourceMessage( KMAccount*, KMMessage* );

  void setMainWin(KMMainWin *mainWin) { mMainWin = mainWin; }
  void setHeaders(KMHeaders* headers );

  // To be exchanged with something reasonable
  void reloadFolderTree() const;

public slots:
  /** View->Groupware menu */
  void slotGroupwareHide();
  /** additional groupware slots */
  void slotGroupwareShow(bool);

  /** Delete and sync the local IMAP cache  */
  void slotInvalidateIMAPFolders();

protected:
  void saveActionEnable( const QString& name, bool on ) const;

  // Figure out if a vCal is a todo, event or neither
  enum VCalType { vCalEvent, vCalTodo, vCalUnknown };
  static VCalType getVCalType( const QString &vCard );

  /** This class functions as an event filter while showing groupware widgets */
  bool eventFilter( QObject *o, QEvent *e ) const;

  // We use QGuardedPtr for everything, since
  // we are not the owner of any of those objects
  QGuardedPtr<KMMainWin>      mMainWin;
  QGuardedPtr<KMHeaders>      mHeaders;
  QGuardedPtr<KMReaderWin>    mReader;
  QGuardedPtr<KMMimePartTree> mMimePartTree;

signals:
  void signalSetKroupwareCommunicationEnabled( QObject* );

  /** Make sure a given time span is visible in the Calendar */
  void signalCalendarUpdateView( const QDateTime&, const QDateTime& );

  void signalShowCalendarView();
  void signalShowContactsView();
  void signalShowNotesView();
  void signalShowTodoView();

  /** Open Groupware to consider accepting/declining an invitation */
  void signalEventRequest( const QCString& receiver, const QString&, bool&,
			   QString&, QString&, bool& );

  /** Use Groupware to create an answer to a resource request. */
  void signalResourceRequest( const QValueList<QPair<QDateTime, QDateTime> >& busy,
                              const QCString& resource,
                              const QString& vCalIn, bool& vCalInOK,
                              QString& vCalOut, bool& vCalOutOK,
                              bool& isFree, QDateTime& start, QDateTime& end );

  /** Accept an invitation without checking: Groupware will *not* show up */
  void signalAcceptedEvent( bool, const QCString&, const QString&, bool&,
			    QString&, bool& );

  /** Reject an invitation: Groupware will *not* show up */
  void signalRejectedEvent( const QCString&, const QString&, bool&, QString&,
			    bool& );

  /** Answer an invitation */
  void signalIncidenceAnswer( const QCString&, const QString&, QString& );

  /** An event was deleted */
  void signalEventDeleted( const QString& );

  /** A task was deleted */
  void signalTaskDeleted( const QString& );

  /** A note was deleted */
  void signalNoteDeleted( const QString& );

  /** The menus were changed */
  void signalMenusChanged();

private:
  void internalCreateKOrgPart();
  void setEnabled( bool b );

  bool mUseGroupware;
  bool mGroupwareIsHidingMimePartTree;
  QSplitter* mPanner;
  QGuardedPtr<KParts::ReadOnlyPart> mKOrgPart;
  QGuardedPtr<QWidget> mKOrgPartParent;
};

#endif /* KMGROUPWARE_H */
