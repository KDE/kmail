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
protected:
  KMMLInfo();
  ~KMMLInfo();
};


#endif

