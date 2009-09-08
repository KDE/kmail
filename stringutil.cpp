/* Copyright 2009 Thomas McGuire <mcguire@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "stringutil.h"

#ifndef KMAIL_UNITTESTS

#include "kmaddrbook.h"
#include "kmkernel.h"

#include <libkdepim/kaddrbookexternal.h>
#include <mimelib/enum.h>

#include <kmime/kmime_charfreq.h>
#include <kmime/kmime_header_parsing.h>
#include <KPIMUtils/Email>
#include <KPIMIdentities/IdentityManager>

#include <kascii.h>
#include <KConfigGroup>
#include <KDebug>
#include <KUrl>
#include <kuser.h>

#include <QHostInfo>
#include <QRegExp>
#endif
#include <QStringList>

#ifndef KMAIL_UNITTESTS

using namespace KMime;
using namespace KMime::Types;
using namespace KMime::HeaderParsing;

#endif

namespace KMail
{

namespace StringUtil
{

static void removeTrailingSpace( QString &line )
{
  int i = line.length()-1;
  while( (i >= 0) && ((line[i] == ' ') || (line[i] == '\t')))
    i--;
  line.truncate( i+1);
}

static QString splitLine( QString &line)
{
  removeTrailingSpace( line );
  int i = 0;
  int j = -1;
  int l = line.length();

  // TODO: Replace tabs with spaces first.

  while(i < l)
  {
      QChar c = line[i];
      if ((c == '>') || (c == ':') || (c == '|'))
        j = i+1;
      else if ((c != ' ') && (c != '\t'))
        break;
      i++;
  }

  if ( j <= 0 )
  {
      return "";
  }
  if ( i == l )
  {
      QString result = line.left(j);
      line.clear();
      return result;
  }

  QString result = line.left(j);
  line = line.mid(j);
  return result;
}


static QString flowText(QString &text, const QString& indent, int maxLength)
{
   maxLength--;
   if (text.isEmpty())
   {
      return indent+"<NULL>\n";
   }
   QString result;
   while (1)
   {
      int i;
      if ((int) text.length() > maxLength)
      {
         i = maxLength;
         while( (i >= 0) && (text[i] != ' '))
            i--;
         if (i <= 0)
         {
            // Couldn't break before maxLength.
            i = maxLength;
//            while( (i < (int) text.length()) && (text[i] != ' '))
//               i++;
         }
      }
      else
      {
         i = text.length();
      }

      QString line = text.left(i);
      if (i < (int) text.length())
         text = text.mid(i);
      else
         text.clear();

      result += indent + line + '\n';

      if (text.isEmpty())
         return result;
   }
}

static bool flushPart(QString &msg, QStringList &part,
                      const QString &indent, int maxLength)
{
   maxLength -= indent.length();
   if (maxLength < 20) maxLength = 20;

   // Remove empty lines at end of quote
   while ((part.begin() != part.end()) && part.last().isEmpty())
   {
      part.removeLast();
   }

   QString text;
   for(QStringList::Iterator it2 = part.begin();
       it2 != part.end();
       ++it2)
   {
      QString line = (*it2);

      if (line.isEmpty())
      {
         if (!text.isEmpty())
            msg += flowText(text, indent, maxLength);
         msg += indent + '\n';
      }
      else
      {
         if (text.isEmpty())
            text = line;
         else
            text += ' '+line.trimmed();

         if (((int) text.length() < maxLength) || ((int) line.length() < (maxLength-10)))
            msg += flowText(text, indent, maxLength);
      }
   }
   if (!text.isEmpty())
      msg += flowText(text, indent, maxLength);

   bool appendEmptyLine = true;
   if (!part.count())
     appendEmptyLine = false;

   part.clear();
   return appendEmptyLine;
}

QString stripSignature ( const QString & msg, bool clearSigned )
{
  // Following RFC 3676, only > before --
  // I prefer to not delete a SB instead of delete good mail content.
  const QRegExp sbDelimiterSearch = clearSigned ?
      QRegExp( "(^|\n)[> ]*--\\s?\n" ) : QRegExp( "(^|\n)[> ]*-- \n" );
  // The regular expression to look for prefix change
  const QRegExp commonReplySearch = QRegExp( "^[ ]*>" );

  QString res = msg;
  int posDeletingStart = 1; // to start looking at 0

  // While there are SB delimiters (start looking just before the deleted SB)
  while ( ( posDeletingStart = res.indexOf( sbDelimiterSearch , posDeletingStart -1 ) ) >= 0 )
  {
    QString prefix; // the current prefix
    QString line; // the line to check if is part of the SB
    int posNewLine = -1;
    int posSignatureBlock = -1;
    // Look for the SB beginning
    posSignatureBlock = res.indexOf( '-', posDeletingStart );
    // The prefix before "-- "$
    if ( res[posDeletingStart] == '\n' ) ++posDeletingStart;
    prefix = res.mid( posDeletingStart, posSignatureBlock - posDeletingStart );
    posNewLine = res.indexOf( '\n', posSignatureBlock ) + 1;

    // now go to the end of the SB
    while ( posNewLine < res.size() && posNewLine > 0 )
    {
      // handle the undefined case for mid ( x , -n ) where n>1
      int nextPosNewLine = res.indexOf( '\n', posNewLine );
      if ( nextPosNewLine < 0 ) nextPosNewLine = posNewLine - 1;
      line = res.mid( posNewLine, nextPosNewLine - posNewLine );

      // check when the SB ends:
      // * does not starts with prefix or
      // * starts with prefix+(any substring of prefix)
      if ( ( prefix.isEmpty() && line.indexOf( commonReplySearch ) < 0 ) ||
             ( !prefix.isEmpty() && line.startsWith( prefix ) &&
             line.mid( prefix.size() ).indexOf( commonReplySearch ) < 0 ) )
      {
        posNewLine = res.indexOf( '\n', posNewLine ) + 1;
      }
      else
        break; // end of the SB
    }
    // remove the SB or truncate when is the last SB
    if ( posNewLine > 0 )
      res.remove( posDeletingStart, posNewLine - posDeletingStart );
    else
      res.truncate( posDeletingStart );
  }
  return res;
}

#ifndef KMAIL_UNITTESTS
//-----------------------------------------------------------------------------
QList<int> determineAllowedCtes( const CharFreq& cf,
                                 bool allow8Bit,
                                 bool willBeSigned )
{
  QList<int> allowedCtes;

  switch ( cf.type() ) {
    case CharFreq::SevenBitText:
      allowedCtes << DwMime::kCte7bit;
    case CharFreq::EightBitText:
      if ( allow8Bit )
        allowedCtes << DwMime::kCte8bit;
    case CharFreq::SevenBitData:
      if ( cf.printableRatio() > 5.0/6.0 ) {
      // let n the length of data and p the number of printable chars.
      // Then base64 \approx 4n/3; qp \approx p + 3(n-p)
      // => qp < base64 iff p > 5n/6.
        allowedCtes << DwMime::kCteQp;
        allowedCtes << DwMime::kCteBase64;
      } else {
        allowedCtes << DwMime::kCteBase64;
        allowedCtes << DwMime::kCteQp;
      }
      break;
    case CharFreq::EightBitData:
      allowedCtes << DwMime::kCteBase64;
      break;
    case CharFreq::None:
    default:
    // just nothing (avoid compiler warning)
      ;
  }

  // In the following cases only QP and Base64 are allowed:
  // - the buffer will be OpenPGP/MIME signed and it contains trailing
  //   whitespace (cf. RFC 3156)
  // - a line starts with "From "
  if ( ( willBeSigned && cf.hasTrailingWhitespace() ) ||
         cf.hasLeadingFrom() ) {
    allowedCtes.removeAll( DwMime::kCte8bit );
    allowedCtes.removeAll( DwMime::kCte7bit );
         }

         return allowedCtes;
}

AddressList splitAddrField( const QByteArray & str )
{
  AddressList result;
  const char * scursor = str.begin();
  if ( !scursor )
    return AddressList();
  const char * const send = str.begin() + str.length();
  if ( !parseAddressList( scursor, send, result ) ) {
    kDebug() << "Error in address splitting: parseAddressList returned false!";
  }
  return result;
}

QString generateMessageId( const QString& addr )
{
  QDateTime datetime = QDateTime::currentDateTime();
  QString msgIdStr;

  msgIdStr = '<' + datetime.toString( "yyyyMMddhhmm.sszzz" );

  QString msgIdSuffix;
  KConfigGroup general( KMKernel::config(), "General" );

  if( general.readEntry( "useCustomMessageIdSuffix", false ) )
    msgIdSuffix = general.readEntry( "myMessageIdSuffix" );

  if( !msgIdSuffix.isEmpty() )
    msgIdStr += '@' + msgIdSuffix;
  else
    msgIdStr += '.' + KPIMUtils::toIdn( addr );

  msgIdStr += '>';

  return msgIdStr;
}
#endif

QByteArray html2source( const QByteArray & src )
{
  QByteArray result( 1 + 6*src.length(), '\0' );  // maximal possible length

  QByteArray::ConstIterator s = src.begin();
  QByteArray::Iterator d = result.begin();
  while ( *s ) {
    switch ( *s ) {
      case '<': {
        *d++ = '&';
        *d++ = 'l';
        *d++ = 't';
        *d++ = ';';
        ++s;
      }
      break;
      case '\r': {
        ++s;
      }
      break;
      case '\n': {
        *d++ = '<';
        *d++ = 'b';
        *d++ = 'r';
        *d++ = '>';
        ++s;
      }
      break;
      case '>': {
        *d++ = '&';
        *d++ = 'g';
        *d++ = 't';
        *d++ = ';';
        ++s;
      }
      break;
      case '&': {
        *d++ = '&';
        *d++ = 'a';
        *d++ = 'm';
        *d++ = 'p';
        *d++ = ';';
        ++s;
      }
      break;
      case '"': {
        *d++ = '&';
        *d++ = 'q';
        *d++ = 'u';
        *d++ = 'o';
        *d++ = 't';
        *d++ = ';';
        ++s;
      }
      break;
      case '\'': {
        *d++ = '&';
        *d++ = 'a';
        *d++ = 'p';
        *d++ = 's';
        *d++ = ';';
        ++s;
      }
      break;
      default:
        *d++ = *s++;
    }
  }
  result.truncate( d - result.begin() );
  return result;
}

#ifndef KMAIL_UNITTESTS
QString encodeMailtoUrl( const QString& str )
{
  QString result;
  result = QString::fromLatin1( KMMsgBase::encodeRFC2047String( str,
                                "utf-8" ) );
  result = KUrl::toPercentEncoding( result );
  return result;
}

QString decodeMailtoUrl( const QString& url )
{
  QString result;
  result = KUrl::fromPercentEncoding( url.toLatin1() );
  result = KMMsgBase::decodeRFC2047String( result.toLatin1() );
  return result;
}
#endif

QByteArray stripEmailAddr( const QByteArray& aStr )
{
  //kDebug() << "(" << aStr << ")";

  if ( aStr.isEmpty() )
    return QByteArray();

  QByteArray result;

  // The following is a primitive parser for a mailbox-list (cf. RFC 2822).
  // The purpose is to extract a displayable string from the mailboxes.
  // Comments in the addr-spec are not handled. No error checking is done.

  QByteArray name;
  QByteArray comment;
  QByteArray angleAddress;
  enum { TopLevel, InComment, InAngleAddress } context = TopLevel;
  bool inQuotedString = false;
  int commentLevel = 0;

  for ( const char* p = aStr.data(); *p; ++p ) {
    switch ( context ) {
      case TopLevel : {
        switch ( *p ) {
          case '"' : inQuotedString = !inQuotedString;
          break;
          case '(' : if ( !inQuotedString ) {
            context = InComment;
            commentLevel = 1;
          }
          else
            name += *p;
          break;
          case '<' : if ( !inQuotedString ) {
            context = InAngleAddress;
          }
          else
            name += *p;
          break;
          case '\\' : // quoted character
            ++p; // skip the '\'
            if ( *p )
              name += *p;
            break;
            case ',' : if ( !inQuotedString ) {
                   // next email address
              if ( !result.isEmpty() )
                result += ", ";
              name = name.trimmed();
              comment = comment.trimmed();
              angleAddress = angleAddress.trimmed();
                   /*
              kDebug() << "Name    : \"" << name
              << "\"";
              kDebug() << "Comment : \"" << comment
              << "\"";
              kDebug() << "Address : \"" << angleAddress
              << "\"";
                   */
              if ( angleAddress.isEmpty() && !comment.isEmpty() ) {
                     // handle Outlook-style addresses like
                     // john.doe@invalid (John Doe)
                result += comment;
              }
              else if ( !name.isEmpty() ) {
                result += name;
              }
              else if ( !comment.isEmpty() ) {
                result += comment;
              }
              else if ( !angleAddress.isEmpty() ) {
                result += angleAddress;
              }
              name = QByteArray();
              comment = QByteArray();
              angleAddress = QByteArray();
            }
            else
              name += *p;
            break;
            default :  name += *p;
        }
        break;
      }
      case InComment : {
        switch ( *p ) {
          case '(' : ++commentLevel;
          comment += *p;
          break;
          case ')' : --commentLevel;
          if ( commentLevel == 0 ) {
            context = TopLevel;
            comment += ' '; // separate the text of several comments
          }
          else
            comment += *p;
          break;
          case '\\' : // quoted character
            ++p; // skip the '\'
            if ( *p )
              comment += *p;
            break;
            default :  comment += *p;
        }
        break;
      }
      case InAngleAddress : {
        switch ( *p ) {
          case '"' : inQuotedString = !inQuotedString;
          angleAddress += *p;
          break;
          case '>' : if ( !inQuotedString ) {
            context = TopLevel;
          }
          else
            angleAddress += *p;
          break;
          case '\\' : // quoted character
            ++p; // skip the '\'
            if ( *p )
              angleAddress += *p;
            break;
            default :  angleAddress += *p;
        }
        break;
      }
    } // switch ( context )
  }
  if ( !result.isEmpty() )
    result += ", ";
  name = name.trimmed();
  comment = comment.trimmed();
  angleAddress = angleAddress.trimmed();
  /*
  kDebug() << "Name    : \"" << name <<"\"";
  kDebug() << "Comment : \"" << comment <<"\"";
  kDebug() << "Address : \"" << angleAddress <<"\"";
  */
  if ( angleAddress.isEmpty() && !comment.isEmpty() ) {
    // handle Outlook-style addresses like
    // john.doe@invalid (John Doe)
    result += comment;
  }
  else if ( !name.isEmpty() ) {
    result += name;
  }
  else if ( !comment.isEmpty() ) {
    result += comment;
  }
  else if ( !angleAddress.isEmpty() ) {
    result += angleAddress;
  }

  //kDebug() << "Returns \"" << result << "\"";
  return result;
}

QString stripEmailAddr( const QString& aStr )
{
  //kDebug() << "(" << aStr << ")";

  if ( aStr.isEmpty() )
    return QString();

  QString result;

  // The following is a primitive parser for a mailbox-list (cf. RFC 2822).
  // The purpose is to extract a displayable string from the mailboxes.
  // Comments in the addr-spec are not handled. No error checking is done.

  QString name;
  QString comment;
  QString angleAddress;
  enum { TopLevel, InComment, InAngleAddress } context = TopLevel;
  bool inQuotedString = false;
  int commentLevel = 0;

  QChar ch;
  int strLength(aStr.length());
  for ( int index = 0; index < strLength; ++index ) {
    ch = aStr[index];
    switch ( context ) {
      case TopLevel : {
        switch ( ch.toLatin1() ) {
          case '"' : inQuotedString = !inQuotedString;
          break;
          case '(' : if ( !inQuotedString ) {
            context = InComment;
            commentLevel = 1;
          }
          else
            name += ch;
          break;
          case '<' : if ( !inQuotedString ) {
            context = InAngleAddress;
          }
          else
            name += ch;
          break;
          case '\\' : // quoted character
            ++index; // skip the '\'
            if ( index < aStr.length() )
              name += aStr[index];
            break;
            case ',' : if ( !inQuotedString ) {
                   // next email address
              if ( !result.isEmpty() )
                result += ", ";
              name = name.trimmed();
              comment = comment.trimmed();
              angleAddress = angleAddress.trimmed();
                   /*
              kDebug() << "Name    : \"" << name
              << "\"";
              kDebug() << "Comment : \"" << comment
              << "\"";
              kDebug() << "Address : \"" << angleAddress
              << "\"";
                   */
              if ( angleAddress.isEmpty() && !comment.isEmpty() ) {
                     // handle Outlook-style addresses like
                     // john.doe@invalid (John Doe)
                result += comment;
              }
              else if ( !name.isEmpty() ) {
                result += name;
              }
              else if ( !comment.isEmpty() ) {
                result += comment;
              }
              else if ( !angleAddress.isEmpty() ) {
                result += angleAddress;
              }
              name.clear();
              comment.clear();
              angleAddress.clear();
            }
            else
              name += ch;
            break;
            default :  name += ch;
        }
        break;
      }
      case InComment : {
        switch ( ch.toLatin1() ) {
          case '(' : ++commentLevel;
          comment += ch;
          break;
          case ')' : --commentLevel;
          if ( commentLevel == 0 ) {
            context = TopLevel;
            comment += ' '; // separate the text of several comments
          }
          else
            comment += ch;
          break;
          case '\\' : // quoted character
            ++index; // skip the '\'
            if ( index < aStr.length() )
              comment += aStr[index];
            break;
            default :  comment += ch;
        }
        break;
      }
      case InAngleAddress : {
        switch ( ch.toLatin1() ) {
          case '"' : inQuotedString = !inQuotedString;
          angleAddress += ch;
          break;
          case '>' : if ( !inQuotedString ) {
            context = TopLevel;
          }
          else
            angleAddress += ch;
          break;
          case '\\' : // quoted character
            ++index; // skip the '\'
            if ( index < aStr.length() )
              angleAddress += aStr[index];
            break;
            default :  angleAddress += ch;
        }
        break;
      }
    } // switch ( context )
  }
  if ( !result.isEmpty() )
    result += ", ";
  name = name.trimmed();
  comment = comment.trimmed();
  angleAddress = angleAddress.trimmed();
  /*
  kDebug() << "Name    : \"" << name <<"\"";
  kDebug() << "Comment : \"" << comment <<"\"";
  kDebug() << "Address : \"" << angleAddress <<"\"";
  */
  if ( angleAddress.isEmpty() && !comment.isEmpty() ) {
    // handle Outlook-style addresses like
    // john.doe@invalid (John Doe)
    result += comment;
  }
  else if ( !name.isEmpty() ) {
    result += name;
  }
  else if ( !comment.isEmpty() ) {
    result += comment;
  }
  else if ( !angleAddress.isEmpty() ) {
    result += angleAddress;
  }

  //kDebug() << "Returns \"" << result << "\"";
  return result;
}

QString quoteHtmlChars( const QString& str, bool removeLineBreaks )
{
  QString result;

  unsigned int strLength(str.length());
  result.reserve( 6*strLength ); // maximal possible length
  for( unsigned int i = 0; i < strLength; ++i ) {
    switch ( str[i].toLatin1() ) {
      case '<':
        result += "&lt;";
        break;
      case '>':
        result += "&gt;";
        break;
      case '&':
        result += "&amp;";
        break;
      case '"':
        result += "&quot;";
        break;
      case '\n':
        if ( !removeLineBreaks )
          result += "<br>";
        break;
      case '\r':
      // ignore CR
        break;
      default:
        result += str[i];
    }
  }

  result.squeeze();
  return result;
}

#ifndef KMAIL_UNITTESTS
QString emailAddrAsAnchor( const QString& aEmail, Display display, const QString& cssStyle,
                             Link link, AddressMode expandable, const QString& fieldName )
{
  if( aEmail.isEmpty() )
    return aEmail;

  const QStringList addressList = KPIMUtils::splitAddressList( aEmail );

  QString result;
  int numberAddresses = 0;
  bool expandableInserted = false;

  for( QStringList::ConstIterator it = addressList.constBegin();
       ( it != addressList.constEnd() );
       ++it ) {
    if( !(*it).isEmpty() ) {
      numberAddresses++;

      QString address = *it;
      if( expandable == ExpandableAddresses && !expandableInserted && numberAddresses > GlobalSettings::self()->numberOfAddressesToShow() ) {
        Q_ASSERT( !fieldName.isEmpty() );
        result = "<span id=\"icon" + fieldName + "\"></span>" + result;
        result += "<span id=\"dots" + fieldName + "\">...</span><span id=\"hidden" + fieldName +"\">";
        expandableInserted = true;
      }
      if( link == ShowLink ) {
        result += "<a href=\"mailto:"
                + encodeMailtoUrl( address )
                + "\" "+cssStyle+">";
      }
      if( display == DisplayNameOnly )
        address = stripEmailAddr( address );
      result += quoteHtmlChars( address, true );
      if( link == ShowLink ) {
        result += "</a>, ";
      }
    }
  }
  // cut of the trailing ", "
  if( link == ShowLink ) {
    result.truncate( result.length() - 2 );
  }
  if( expandableInserted ) {
    result += "</span>";
  }

  //kDebug() << "('" << aEmail << "') returns:\n-->" << result << "<--";
  return result;
}

QStringList stripAddressFromAddressList( const QString& address,
                                         const QStringList& list )
{
  QStringList addresses( list );
  QString addrSpec( KPIMUtils::extractEmailAddress( address ) );
  for ( QStringList::Iterator it = addresses.begin();
        it != addresses.end(); ) {
    if ( kasciistricmp( addrSpec.toUtf8().data(),
         KPIMUtils::extractEmailAddress( *it ).toUtf8().data() ) == 0 ) {
      kDebug() << "Removing" << *it << "from the address list";
      it = addresses.erase( it );
    }
    else
      ++it;
  }
  return addresses;
}

QStringList stripMyAddressesFromAddressList( const QStringList& list )
{
  QStringList addresses = list;
  for( QStringList::Iterator it = addresses.begin();
       it != addresses.end(); ) {
    kDebug() << "Check whether" << *it <<"is one of my addresses";
    if( kmkernel->identityManager()->thatIsMe( KPIMUtils::extractEmailAddress( *it ) ) ) {
      kDebug() << "Removing" << *it <<"from the address list";
      it = addresses.erase( it );
    }
    else
      ++it;
  }
  return addresses;
}

bool addressIsInAddressList( const QString& address,
                             const QStringList& addresses )
{
  QString addrSpec = KPIMUtils::extractEmailAddress( address );
  for( QStringList::ConstIterator it = addresses.begin();
       it != addresses.end(); ++it ) {
    if ( kasciistricmp( addrSpec.toUtf8().data(),
         KPIMUtils::extractEmailAddress( *it ).toUtf8().data() ) == 0 )
      return true;
  }
  return false;
}

QString expandAliases( const QString& recipients, QStringList &distributionListEmpty )
{
  if ( recipients.isEmpty() )
    return QString();

  QStringList recipientList = KPIMUtils::splitAddressList( recipients );
  QString expandedRecipients;
  for ( QStringList::Iterator it = recipientList.begin();
        it != recipientList.end(); ++it ) {
    if ( !expandedRecipients.isEmpty() )
      expandedRecipients += ", ";
    QString receiver = (*it).trimmed();

    // try to expand distribution list
    bool distributionListIsEmpty = false;
    QString expandedList = KPIM::KAddrBookExternal::expandDistributionList( receiver, distributionListIsEmpty );
    if ( distributionListIsEmpty ) {
      expandedRecipients += receiver;
      distributionListEmpty << receiver;
      continue;
    }

    if ( !expandedList.isEmpty()) {
      expandedRecipients += expandedList;
      continue;
    }

    // try to expand nick name
    QString expandedNickName = KabcBridge::expandNickName( receiver );
    if ( !expandedNickName.isEmpty() ) {
      expandedRecipients += expandedNickName;
      continue;
    }

    // check whether the address is missing the domain part
    QByteArray displayName, addrSpec, comment;
    KPIMUtils::splitAddress( receiver.toLatin1(), displayName, addrSpec, comment );
    if ( !addrSpec.contains('@') ) {
      KConfigGroup general( KMKernel::config(), "General" );
      QString defaultdomain = general.readEntry( "Default domain" );
      if ( !defaultdomain.isEmpty() ) {
        expandedRecipients += KPIMUtils::normalizedAddress( displayName, addrSpec + '@' + defaultdomain, comment );
      }
      else {
        expandedRecipients += guessEmailAddressFromLoginName( addrSpec );
      }
    }
    else
      expandedRecipients += receiver;
  }

  return expandedRecipients;
}

QString guessEmailAddressFromLoginName( const QString& loginName )
{
  if ( loginName.isEmpty() )
    return QString();

  QString address = loginName;
  address += '@';
  address += QHostInfo::localHostName();

  // try to determine the real name
  const KUser user( loginName );
  if ( user.isValid() ) {
    QString fullName = user.property( KUser::FullName ).toString();
    if ( fullName.contains( QRegExp( "[^ 0-9A-Za-z\\x0080-\\xFFFF]" ) ) )
      address = '"' + fullName.replace( '\\', "\\" ).replace( '"', "\\" )
          + "\" <" + address + '>';
    else
      address = fullName + " <" + address + '>';
  }

  return address;
}
#endif

QString smartQuote( const QString & msg, int maxLineLength )
{
  QStringList part;
  QString oldIndent;
  bool firstPart = true;

  const QStringList lines = msg.split('\n');

  QString result;
  for(QStringList::const_iterator it = lines.begin();
      it != lines.end();
      ++it)
  {
     QString line = *it;

     const QString indent = splitLine( line );

     if ( line.isEmpty())
     {
        if (!firstPart)
           part.append(QString());
        continue;
     };

     if (firstPart)
     {
        oldIndent = indent;
        firstPart = false;
     }

     if (oldIndent != indent)
     {
        QString fromLine;
        // Search if the last non-blank line could be "From" line
        if (part.count() && (oldIndent.length() < indent.length()))
        {
           QStringList::Iterator it2 = part.isEmpty() ? part.end() : --part.end();
           // FIXME: what if all strings are empty? Then we'll decrement part.begin().
           // Shouldn't we also check for .begin()?
           while( (it2 != part.end()) && (*it2).isEmpty())
             --it2;

           if ((it2 != part.end()) && ((*it2).endsWith(':')))
           {
              fromLine = oldIndent + (*it2) + '\n';
              part.erase(it2);
           }
        }
        if (flushPart( result, part, oldIndent, maxLineLength))
        {
           if (oldIndent.length() > indent.length())
              result += indent + '\n';
           else
              result += oldIndent + '\n';
        }
        if (!fromLine.isEmpty())
        {
           result += fromLine;
        }
        oldIndent = indent;
     }
     part.append(line);
  }
  flushPart( result, part, oldIndent, maxLineLength);
  return result;
}

QString formatString( const QString &wildString, const QString &fromAddr )
{
  QString result;

  if ( wildString.isEmpty() ) {
    return wildString;
  }

  unsigned int strLength( wildString.length() );
  for ( uint i=0; i<strLength; ) {
    QChar ch = wildString[i++];
    if ( ch == '%' && i<strLength ) {
      ch = wildString[i++];
      switch ( ch.toLatin1() ) {
      case 'f': // sender's initals
      {
        QString str = stripEmailAddr( fromAddr );

        uint j = 0;
        for ( ; str[j]>' '; j++ )
          ;
        unsigned int strLength( str.length() );
        for ( ; j < strLength && str[j] <= ' '; j++ )
          ;
        result += str[0];
        if ( str[j] > ' ' ) {
          result += str[j];
        } else {
          if ( str[1] > ' ' ) {
            result += str[1];
          }
        }
      }
      break;
      case '_':
        result += ' ';
        break;
      case '%':
        result += '%';
        break;
      default:
        result += '%';
        result += ch;
        break;
      }
    } else {
      result += ch;
    }
  }
  return result;
}

#ifndef KMAIL_UNITTESTS

void parseMailtoUrl ( const KUrl& url, QString& to, QString& cc, QString& subject, QString& body )
{
  to = decodeMailtoUrl( url.path() );
  body = url.queryItem( "body" );
  subject = url.queryItem( "subject" );
  kDebug() << url.pathOrUrl();
  cc = url.queryItem( "cc" );
}

#endif

bool isCryptoPart( const QString &type, const QString &subType, const QString &fileName )
{
  return ( type.toLower() == "application" &&
           ( subType.toLower() == "pgp-encrypted" ||
             subType.toLower() == "pgp-signature" ||
             subType.toLower() == "pkcs7-mime" ||
             subType.toLower() == "pkcs7-signature" ||
             subType.toLower() == "x-pkcs7-signature" ||
             ( subType.toLower() == "octet-stream" &&
               fileName.toLower() == "msg.asc" ) ) );
}

}

}
