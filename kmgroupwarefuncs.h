#ifndef KMGROUPWAREFUNCS_H
#define KMGROUPWAREFUNCS_H

#include <qstring.h>
#include <qdatetime.h>


struct save_tz { char* old_tz; char* tz_env_str; };

/* temporarily go to a different timezone */
struct save_tz set_tz( const char* _tc );

/* restore previous timezone */
void unset_tz( struct save_tz old_tz );

QDateTime utc2Local( const QDateTime& utcdt );

QDateTime pureISOToLocalQDateTime(const QString & dtStr, bool bDateOnly=false);

QString ISOToLocalQDateTime(const QString & dtStr);

// This is a very light-weight and fast 'parser' to retrieve up
// to 7 data entries from a vCal taking continuation lines
// into account
// This very primitive function may be removed once a link
// to an iCal/vCal parsing library is established...
extern QString nullQString;
void vPartMicroParser( const QCString& str, QString& s1, QString& s2=nullQString,
			      QString& s3=nullQString, QString& s4=nullQString,
			      QString& s5=nullQString, QString& s6=nullQString,
			      QString& s7=nullQString );

QString isoDateTimeToLocal(const QString& isoStr );

// This replaces chars with the html equivalents
void string2HTML( QString& str );

#endif // KMGROUPWAREFUNCS_H
