/* Message Property
 *
 * Author: Don Sanders <sanders@kde.org>
 * License: GPL
 */
#ifndef messageproperty_h
#define messageproperty_h

#include "kmfilteraction.h" // for KMFilterAction::ReturnCode
#include "kmfolder.h"

#include <qptrlist.h>
#include <qguardedptr.h>
#include <qobject.h>

class KMFilter;
class KMFilterDlg;

namespace KMail {

class ActionScheduler;

/* A place to store properties that some but not necessarily all messages
   have.

   These properties do not need to be stored persistantly on disk but
   rather only are required while KMail is running.

   Furthermore some properties, namely complete, transferInProgress, and
   serialCache should only exist during the lifetime of a particular
   KMMsgBase based instance.
 */
class MessageProperty : public QObject
{
  Q_OBJECT

public:
  /** If the message is being filtered  */
  static bool filtering( Q_UINT32 );
  static void setFiltering( Q_UINT32, bool filtering );
  static bool filtering( const KMMsgBase* );
  static void setFiltering( const KMMsgBase*, bool filtering );
  /** The folder this message is to be moved into once
      filtering is finished, or null if the message is not
      scheduled to be moved */
  static KMFolder* filterFolder( Q_UINT32 );
  static void setFilterFolder( Q_UINT32, KMFolder* folder );
  static KMFolder* filterFolder( const KMMsgBase* );
  static void setFilterFolder( const KMMsgBase*, KMFolder* folder );
  /* Set the filterHandler for a message */
  static ActionScheduler* filterHandler( Q_UINT32 );
  static void setFilterHandler( Q_UINT32, ActionScheduler* filterHandler );
  static ActionScheduler* filterHandler( const KMMsgBase* );
  static void setFilterHandler( const KMMsgBase*, ActionScheduler* filterHandler );

  /* Caches the serial number for a message, or more correctly for a
     KMMsgBase based instance representing a message.
     This property becomes invalid when the message is destructed or
     assigned a new value */
  static void setSerialCache( const KMMsgBase*, Q_UINT32 );
  static Q_UINT32 serialCache( const KMMsgBase* );
  /* Set the completeness for a message
     This property becomes invalid when the message is destructed or
     assigned a new value */
  static void setComplete( const KMMsgBase*, bool );
  static bool complete( const KMMsgBase* );
  static void setComplete( Q_UINT32, bool );
  static bool complete( Q_UINT32 );
  /* Set the transferInProgress for a message
     This property becomes invalid when the message is destructed or
     assigned a new value */
  static void setTransferInProgress( const KMMsgBase*, bool, bool = false );
  static bool transferInProgress( const KMMsgBase* );
  static void setTransferInProgress( Q_UINT32, bool, bool = false );
  static bool transferInProgress( Q_UINT32 );
  /** Some properties, namely complete, transferInProgress, and
      serialCache must be forgotten when a message class instance is
      destructed or assigned a new value */
  static void forget( const KMMsgBase* );

private:
  // The folder a message is to be moved into once filtering is finished if any
  static QMap<Q_UINT32, QGuardedPtr<KMFolder> > sFolders;
  // The action scheduler currently processing a message if any
  static QMap<Q_UINT32, QGuardedPtr<ActionScheduler> > sHandlers;
  // The completeness state of a message if any
  static QMap<Q_UINT32, bool > sCompletes;
  // The transferInProgres state of a message if any.
  static QMap<Q_UINT32, int > sTransfers;
  // The cached serial number of a message if any.
  static QMap<const KMMsgBase*, long > sSerialCache;
};

}

#endif /*messageproperty_h*/
