#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmgroupwarefuncs.h"

#include <stdlib.h>
#include <time.h>


/* temporarily go to a different timezone */
struct save_tz set_tz( const char* _tc )
{
  const char *tc = _tc?_tc:"UTC";

  struct save_tz rv;

  rv.old_tz = 0;
  rv.tz_env_str = 0;

  //kdDebug(5006) << "set_tz(), timezone before = " << timezone << endl;

  char* tz_env = 0;
  if( getenv( "TZ" ) ) {
    tz_env = strdup( getenv( "TZ" ) );
    rv.old_tz = tz_env;
  }
  char* tmp_env = (char*)malloc( strlen( tc ) + 4 );
  strcpy( tmp_env, "TZ=" );
  strcpy( tmp_env+3, tc );
  putenv( tmp_env );

  rv.tz_env_str = tmp_env;

  /* tmp_env is not free'ed -- it is part of the environment */

  tzset();
  //kdDebug(5006) << "set_tz(), timezone after = " << timezone << endl;

  return rv;
}

/* restore previous timezone */
void unset_tz( struct save_tz old_tz )
{
  if( old_tz.old_tz ) {
    char* tmp_env = (char*)malloc( strlen( old_tz.old_tz ) + 4 );
    strcpy( tmp_env, "TZ=" );
    strcpy( tmp_env+3, old_tz.old_tz );
    putenv( tmp_env );
    /* tmp_env is not free'ed -- it is part of the environment */
    free( old_tz.old_tz );
  } else {
    /* clear TZ from env */
    putenv( strdup("TZ") );
  }
  tzset();

  /* is this OK? */
  if( old_tz.tz_env_str ) free( old_tz.tz_env_str );
}

QDateTime utc2Local( const QDateTime& utcdt )
{
  struct tm tmL;

  save_tz tmp_tz = set_tz("UTC");
  time_t utc = utcdt.toTime_t();
  unset_tz( tmp_tz );

  localtime_r( &utc, &tmL );
  return QDateTime( QDate( tmL.tm_year+1900, tmL.tm_mon+1, tmL.tm_mday ),
                    QTime( tmL.tm_hour, tmL.tm_min, tmL.tm_sec ) );
}

QDateTime pureISOToLocalQDateTime( const QString& dtStr, bool bDateOnly )
{
  QDate tmpDate;
  QTime tmpTime;
  int year, month, day, hour, minute, second;

  if( bDateOnly ){
    year   = dtStr.left(4).toInt();
    month  = dtStr.mid(4,2).toInt();
    day    = dtStr.mid(6,2).toInt();
    hour   = 0;
    minute = 0;
    second = 0;
  }else{
    year   = dtStr.left(4).toInt();
    month  = dtStr.mid(4,2).toInt();
    day    = dtStr.mid(6,2).toInt();
    hour   = dtStr.mid(9,2).toInt();
    minute = dtStr.mid(11,2).toInt();
    second = dtStr.mid(13,2).toInt();
  }
  tmpDate.setYMD(year, month, day);
  tmpTime.setHMS(hour, minute, second);

  if( tmpDate.isValid() && tmpTime.isValid() ){
    QDateTime dT = QDateTime(tmpDate, tmpTime);

    if( !bDateOnly ){
      // correct for GMT ( == Zulu time == UTC )
      if (dtStr.at(dtStr.length()-1) == 'Z'){
        //dT = dT.addSecs( 60 * KRFCDate::localUTCOffset() );//localUTCOffset( dT ) );
        dT = utc2Local( dT );
      }
    }
    return dT;
  }else
    return QDateTime();
}


QString ISOToLocalQDateTime( const QString& dtStr )
{
  const QString sDateFlag("VALUE=DATE:");

  QString tmpStr = dtStr.upper();
  const bool bDateOnly = tmpStr.startsWith( sDateFlag );
  if( bDateOnly )
    tmpStr = tmpStr.remove( sDateFlag );
  QDateTime dT( pureISOToLocalQDateTime(tmpStr, bDateOnly) );
  if( dT.isValid() )
    tmpStr = dT.toString( Qt::ISODate ) + '@' + dT.toString( Qt::LocalDate );
  else
    tmpStr = "?@?";
  return tmpStr;
}


// This is a very light-weight and fast 'parser' to retrieve up
// to 7 data entries from a vCal taking continuation lines
// into account
// This very primitive function may be removed once a link
// to an iCal/vCal parsing library is established...
QString nullQString;
void vPartMicroParser( const QCString& str, QString& s1, QString& s2, QString& s3,
		       QString& s4, QString& s5, QString& s6, QString& s7 )
{
  QString line;
  uint len = str.length();

  // "b1 (or b2..b7, resp.) == true"  means  "seach for s1 (or s2..s7, resp.)"
  bool b1 = true, b2 = (&s2!=&nullQString), b3 = (&s3!=&nullQString);
  bool b4 = (&s4!=&nullQString), b5 = (&s5!=&nullQString), b6 = (&s6!=&nullQString);
  bool b7 = (&s7!=&nullQString);

  for( uint i=0; i<len && (b1 || b2 || b3 || b4 || b5 || b6 || b7 ); ++i){
    if( str[i] == '\r' || str[i] == '\n' ){
      if( str[i] == '\r' )
        ++i;
      if( i+1 < len && str[i+1] == ' ' ){
        // found a continuation line, skip it's leading blanc
        ++i;
      }else{
        // found a logical line end, process the line
        if( b1 && line.startsWith( s1 ) ){
          s1 = line.mid( s1.length() + 1 );
          b1 = false;
        }else if( b2 && line.startsWith( s2 ) ){
          s2 = line.mid( s2.length() + 1 );
          b2 = false;
        }else if( b3 && line.startsWith( s3 ) ){
          s3 = line.mid( s3.length() + 1 );
          b3 = false;
        }else if( b4 && line.startsWith( s4 ) ){
          s4 = line.mid( s4.length() + 1 );
          b4 = false;
        }else if( b5 && line.startsWith( s5 ) ){
          s5 = line.mid( s5.length() + 1 );
          b5 = false;
        }else if( b6 && line.startsWith( s6 ) ){
          s6 = line.mid( s6.length() + 1 );
          b6 = false;
        }else if( b7 && line.startsWith( s7 ) ){
          s7 = line.mid( s7.length() + 1 );
          b7 = false;
        }
        line = "";
      }
    }else{
      line += str[i];
    }
  }
  if( b1 )
    s1.truncate(0);
  if( b2 )
    s2.truncate(0);
  if( b3 )
    s3.truncate(0);
  if( b4 )
    s4.truncate(0);
  if( b5 )
    s5.truncate(0);
  if( b6 )
    s6.truncate(0);
  if( b7 )
    s7.truncate(0);
}

QString isoDateTimeToLocal(const QString& isoStr )
{
  const QDateTime dt( QDateTime::fromString( isoStr, Qt::ISODate ) );
  if( dt.time() == QTime(0,0,0) )
    return dt.date().toString( Qt::LocalDate );
  return dt.toString( Qt::LocalDate );
}

void string2HTML( QString& str ) {
  str.replace( QChar('&'), "&amp;" );
  str.replace( QChar('<'), "&lt;" );
  str.replace( QChar('>'), "&gt;" );
  str.replace( QChar('\"'), "&quot;" );
  str.replace( "\\n", "<br>" );
  str.replace( "\\,", "," );
}
