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

#include <qguardedptr.h>

class KMAccount;
class KMMessage;
class KMReaderWin;
class KMMainWidget;
class KURL;


class KMGroupware : public QObject
{
  Q_OBJECT

public:
  KMGroupware( QObject* parent = 0, const char* name = 0 );
  virtual ~KMGroupware();


public:
  void processVCalRequest( const QCString& receiver, const QString& vCalIn,
                           QString& choice );
  void processVCalReply( const QCString& sender, const QString& vCalIn,
			 const QString& choice );

  /* (Re-)Read configuration file */
  void readConfig();

  bool isEnabled() const { return mUseGroupware; }

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
  bool vPartToHTML( int aUpdateCounter, const QString& vCal, QString fname,
		    QString& prefix, QString& postfix ) const;
  static bool msTNEFToVPart( const QByteArray& tnef, QString& aVPart );
  bool msTNEFToHTML( KMReaderWin* reader, QString& vPart, QString fname,
		     QString& prefix, QString& postfix ) const;

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

  // To be exchanged with something reasonable
  void reloadFolderTree() const;

  void setMainWidget( KMMainWidget* mw ) { mMainWidget = mw; }

protected:
  // Figure out if a vCal is a todo, event or neither
  enum VCalType { vCalEvent, vCalTodo, vCalUnknown };
  static VCalType getVCalType( const QString &vCard );

  void setEnabled( bool b );

  bool mUseGroupware;

  QGuardedPtr<KMMainWidget> mMainWidget;
};

#endif /* KMGROUPWARE_H */
