#ifndef MAILING_LIST_MAGIC_H
#define MAILING_LIST_MAGIC_H

#include <qstring.h>

class KMMessage;

QString detect_list(const KMMessage  *message,
                    QCString &header_name,
                    QString &header_value );

#endif

