#ifndef MAILINGLIST_MAGIC_H
#define MAILINGLIST_MAGIC_H

#include <kurl.h>
#include <qstring.h>

class KMMessage;

class KMMLInfo 
{
public:
  static QString name( const KMMessage  *message, QCString &header_name,
		       QString &header_value );
  static KURL postAddress( const KMMessage  *message );
  static KURL helpAddress( const KMMessage *message );
  static KURL subscribeAddress( const KMMessage *message );
  static KURL usubscribeAddres( const KMMessage *message );
  static KURL archiveAddress( const KMMessage *message );
  static QString listID( const KMMessage *message );
protected:
  KMMLInfo();
  ~KMMLInfo();
  //either "mailto:" or "http:"
  static QString headerToAddress( const QString& header );
};


#endif

