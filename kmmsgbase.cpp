// kmmsgbase.cpp

#include <config.h>

#include "kmmsgbase.h"

#include "kmfolderindex.h"
#include "kmfolder.h"
#include "kmheaders.h"
#include "kmmsgdict.h"
#include "messageproperty.h"
using KMail::MessageProperty;

#include <kdebug.h>
#include <kglobal.h>
#include <kcharsets.h>
#include <kmdcodec.h>
#include <krfcdate.h>

#include <mimelib/mimepp.h>
#include <kmime_codecs.h>

#include <qtextcodec.h>
#include <qdeepcopy.h>
#include <qregexp.h>

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#endif

// We define functions as kmail_swap_NN so that we don't get compile errors
// on platforms where bswap_NN happens to be a function instead of a define.

/* Swap bytes in 16 bit value.  */
#ifdef bswap_16
#define kmail_swap_16(x) bswap_16(x)
#else
#define kmail_swap_16(x) \
     ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))
#endif

/* Swap bytes in 32 bit value.  */
#ifdef bswap_32
#define kmail_swap_32(x) bswap_32(x)
#else
#define kmail_swap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |		      \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
#endif

/* Swap bytes in 64 bit value.  */
#ifdef bswap_64
#define kmail_swap_64(x) bswap_64(x)
#else
#define kmail_swap_64(x) \
     ((((x) & 0xff00000000000000ull) >> 56)				      \
      | (((x) & 0x00ff000000000000ull) >> 40)				      \
      | (((x) & 0x0000ff0000000000ull) >> 24)				      \
      | (((x) & 0x000000ff00000000ull) >> 8)				      \
      | (((x) & 0x00000000ff000000ull) << 8)				      \
      | (((x) & 0x0000000000ff0000ull) << 24)				      \
      | (((x) & 0x000000000000ff00ull) << 40)				      \
      | (((x) & 0x00000000000000ffull) << 56))
#endif

//-----------------------------------------------------------------------------
KMMsgBase::KMMsgBase(KMFolder* aParentFolder)
  : mParent( aParentFolder ), mDirty( false), mIndexOffset( 0 ),
    mIndexLength( 0 ), mEnableUndo( false ), mStatus( KMMsgStatusUnknown )
{
}


//-----------------------------------------------------------------------------
KMMsgBase::~KMMsgBase()
{
  MessageProperty::forget( this );
}

KMFolderIndex* KMMsgBase::storage() const
{
  // TODO: How did this ever work? What about KMFolderSearch that does
  // not inherit KMFolderIndex?
  if( mParent )
    return static_cast<KMFolderIndex*>( mParent->storage() );
  return 0;
}

//-----------------------------------------------------------------------------
void KMMsgBase::assign(const KMMsgBase* other)
{
  mParent = other->mParent;
  mDirty  = other->mDirty;
  mIndexOffset = other->mIndexOffset;
  mIndexLength = other->mIndexLength;
}

//-----------------------------------------------------------------------------
KMMsgBase& KMMsgBase::operator=(const KMMsgBase& other)
{
  assign(&other);
  return *this;
}


//----------------------------------------------------------------------------
KMMsgBase::KMMsgBase( const KMMsgBase& other )
{
    assign( &other );
}

//-----------------------------------------------------------------------------
bool KMMsgBase::isMessage(void) const
{
  return FALSE;
}
//-----------------------------------------------------------------------------
void KMMsgBase::toggleStatus(const KMMsgStatus aStatus, int idx)
{
  mDirty = true;
  KMMsgStatus oldStatus = status();
    if ( status() & aStatus ) {
    mStatus &= ~aStatus;
  } else {
    mStatus |= aStatus;
    // Ignored and Watched are toggleable, yet mutually exclusive.
    // That is an arbitrary restriction on my part. HAR HAR HAR :) -till
    if (aStatus == KMMsgStatusWatched)
      mStatus &= ~KMMsgStatusIgnored;
    if (aStatus == KMMsgStatusIgnored) {
      mStatus &= ~KMMsgStatusWatched;
      // Set to read. Don't use setStatus, that emits msgStatusChanged
      // and we only want to do that once.
      mStatus &= ~KMMsgStatusUnread;
      mStatus &= ~KMMsgStatusNew;
      mStatus |= KMMsgStatusRead;
    }
    if (aStatus == KMMsgStatusSpam)
      mStatus &= ~KMMsgStatusHam;
    if (aStatus == KMMsgStatusHam)
      mStatus &= ~KMMsgStatusSpam;
  }
  if (storage()) {
     if (idx < 0)
       idx = storage()->find( this );
     storage()->msgStatusChanged( oldStatus, status(), idx );
     storage()->headerOfMsgChanged(this, idx);
  }

}

//-----------------------------------------------------------------------------
void KMMsgBase::setStatus(const KMMsgStatus aStatus, int idx)
{
  mDirty = TRUE;
  KMMsgStatus oldStatus = status();
  switch (aStatus) {
    case KMMsgStatusRead:
      // Unset unread and new, set read
      mStatus &= ~KMMsgStatusUnread;
      mStatus &= ~KMMsgStatusNew;
      mStatus |= KMMsgStatusRead;
      break;

    case KMMsgStatusUnread:
      // unread overrides read
      mStatus &= ~KMMsgStatusOld;
      mStatus &= ~KMMsgStatusRead;
      mStatus &= ~KMMsgStatusNew;
      mStatus |= KMMsgStatusUnread;
      break;

    case KMMsgStatusOld:
      // old can't be new or unread
      mStatus &= ~KMMsgStatusNew;
      mStatus &= ~KMMsgStatusUnread;
      mStatus |= KMMsgStatusOld;
      break;

    case KMMsgStatusNew:
      // new overrides old and read
      mStatus &= ~KMMsgStatusOld;
      mStatus &= ~KMMsgStatusRead;
      mStatus &= ~KMMsgStatusUnread;
      mStatus |= KMMsgStatusNew;
      break;

    case KMMsgStatusDeleted:
      mStatus |= KMMsgStatusDeleted;
      break;

    case KMMsgStatusReplied:
      mStatus |= KMMsgStatusReplied;
      break;

    case KMMsgStatusForwarded:
      mStatus |= KMMsgStatusForwarded;
      break;

    case KMMsgStatusQueued:
      mStatus |= KMMsgStatusQueued;
      break;

    case KMMsgStatusTodo:
      mStatus |= KMMsgStatusTodo;
      break;

    case KMMsgStatusSent:
      mStatus &= ~KMMsgStatusQueued;
      mStatus &= ~KMMsgStatusUnread;
      mStatus &= ~KMMsgStatusNew;
      mStatus |= KMMsgStatusSent;
      break;

    case KMMsgStatusFlag:
      mStatus |= KMMsgStatusFlag;
      break;

    // Watched and ignored are mutually exclusive
    case KMMsgStatusWatched:
      mStatus &= ~KMMsgStatusIgnored;
      mStatus |= KMMsgStatusWatched;
      break;

    case KMMsgStatusIgnored:
      mStatus &= ~KMMsgStatusWatched;
      mStatus |= KMMsgStatusIgnored;
      break;
    // as are ham and spam
    case KMMsgStatusSpam:
      mStatus &= ~KMMsgStatusHam;
      mStatus |= KMMsgStatusSpam;
      break;
    case KMMsgStatusHam:
      mStatus &= ~KMMsgStatusSpam;
      mStatus |= KMMsgStatusHam;
      break;
    case KMMsgStatusHasAttach:
      mStatus &= ~KMMsgStatusHasNoAttach;
      mStatus |= KMMsgStatusHasAttach;
      break;
    case KMMsgStatusHasNoAttach:
      mStatus &= ~KMMsgStatusHasAttach;
      mStatus |= KMMsgStatusHasNoAttach;
      break;
    default:
      mStatus = aStatus;
      break;
  }

  if (storage()) {
    if (idx < 0)
      idx = storage()->find( this );
    storage()->msgStatusChanged( oldStatus, status(), idx );
    storage()->headerOfMsgChanged( this, idx );
  }
}



//-----------------------------------------------------------------------------
void KMMsgBase::setStatus(const char* aStatusStr, const char* aXStatusStr)
{
  // first try to find status from "X-Status" field if given
  if (aXStatusStr) {
    if (strchr(aXStatusStr, 'N')) setStatus(KMMsgStatusNew);
    if (strchr(aXStatusStr, 'U')) setStatus(KMMsgStatusUnread);
    if (strchr(aXStatusStr, 'O')) setStatus(KMMsgStatusOld);
    if (strchr(aXStatusStr, 'R')) setStatus(KMMsgStatusRead);
    if (strchr(aXStatusStr, 'D')) setStatus(KMMsgStatusDeleted);
    if (strchr(aXStatusStr, 'A')) setStatus(KMMsgStatusReplied);
    if (strchr(aXStatusStr, 'F')) setStatus(KMMsgStatusForwarded);
    if (strchr(aXStatusStr, 'Q')) setStatus(KMMsgStatusQueued);
    if (strchr(aXStatusStr, 'K')) setStatus(KMMsgStatusTodo);
    if (strchr(aXStatusStr, 'S')) setStatus(KMMsgStatusSent);
    if (strchr(aXStatusStr, 'G')) setStatus(KMMsgStatusFlag);
    if (strchr(aXStatusStr, 'P')) setStatus(KMMsgStatusSpam);
    if (strchr(aXStatusStr, 'H')) setStatus(KMMsgStatusHam);
    if (strchr(aXStatusStr, 'T')) setStatus(KMMsgStatusHasAttach);
    if (strchr(aXStatusStr, 'C')) setStatus(KMMsgStatusHasNoAttach);
  }

  // Merge the contents of the "Status" field
  if (aStatusStr) {
    if ((aStatusStr[0]== 'R' && aStatusStr[1]== 'O') ||
        (aStatusStr[0]== 'O' && aStatusStr[1]== 'R')) {
      setStatus( KMMsgStatusOld );
      setStatus( KMMsgStatusRead );
    }
    else if (aStatusStr[0] == 'R')
      setStatus(KMMsgStatusRead);
    else if (aStatusStr[0] == 'D')
      setStatus(KMMsgStatusDeleted);
    else
      setStatus(KMMsgStatusNew);
  }
}


void KMMsgBase::setEncryptionState( const KMMsgEncryptionState /*status*/, int idx )
{
    //kdDebug(5006) << "***setEncryptionState1( " << status << " )" << endl;
    mDirty = TRUE;
    if (storage())
        storage()->headerOfMsgChanged(this, idx);
}

void KMMsgBase::setEncryptionStateChar( QChar status, int idx )
{
    //kdDebug(5006) << "***setEncryptionState2( " << (status.isNull() ? '?' : status.latin1()) << " )" << endl;

    if( status.latin1() == (char)KMMsgEncryptionStateUnknown )
        setEncryptionState( KMMsgEncryptionStateUnknown, idx );
    else if( status.latin1() == (char)KMMsgNotEncrypted )
        setEncryptionState( KMMsgNotEncrypted, idx );
    else if( status.latin1() == (char)KMMsgPartiallyEncrypted )
        setEncryptionState( KMMsgPartiallyEncrypted, idx );
    else if( status.latin1() == (char)KMMsgFullyEncrypted )
        setEncryptionState( KMMsgFullyEncrypted, idx );
    else
        setEncryptionState( KMMsgEncryptionStateUnknown, idx );
}


void KMMsgBase::setSignatureState( const KMMsgSignatureState /*status*/, int idx )
{
    //kdDebug(5006) << "***setSignatureState1( " << status << " )" << endl;
    mDirty = TRUE;
    if (storage())
         storage()->headerOfMsgChanged(this, idx);
}

void KMMsgBase::setMDNSentState( KMMsgMDNSentState, int idx ) {
  mDirty = true;
  if ( storage() )
    storage()->headerOfMsgChanged(this, idx);
}

void KMMsgBase::setSignatureStateChar( QChar status, int idx )
{
    //kdDebug(5006) << "***setSignatureState2( " << (status.isNull() ? '?' : status.latin1()) << " )" << endl;

    if( status.latin1() == (char)KMMsgSignatureStateUnknown )
        setSignatureState( KMMsgSignatureStateUnknown, idx );
    else if( status.latin1() == (char)KMMsgNotSigned )
        setSignatureState( KMMsgNotSigned, idx );
    else if( status.latin1() == (char)KMMsgPartiallySigned )
        setSignatureState( KMMsgPartiallySigned,idx );
    else if( status.latin1() == (char)KMMsgFullySigned )
        setSignatureState( KMMsgFullySigned, idx );
    else
        setSignatureState( KMMsgSignatureStateUnknown, idx );
}

//-----------------------------------------------------------------------------
bool KMMsgBase::isUnread(void) const
{
  KMMsgStatus st = status();
  return (st & KMMsgStatusUnread);
}

//-----------------------------------------------------------------------------
bool KMMsgBase::isNew(void) const
{
  KMMsgStatus st = status();
  return (st & KMMsgStatusNew);
}

//-----------------------------------------------------------------------------
bool KMMsgBase::isOfUnknownStatus(void) const
{
  KMMsgStatus st = status();
  return (st == KMMsgStatusUnknown);
}

//-----------------------------------------------------------------------------
bool KMMsgBase::isOld(void) const
{
  KMMsgStatus st = status();
  return (st & KMMsgStatusOld);
}

//-----------------------------------------------------------------------------
bool KMMsgBase::isRead(void) const
{
  KMMsgStatus st = status();
  return (st & KMMsgStatusRead);
}

//-----------------------------------------------------------------------------
bool KMMsgBase::isDeleted(void) const
{
  KMMsgStatus st = status();
  return (st & KMMsgStatusDeleted);
}

//-----------------------------------------------------------------------------
bool KMMsgBase::isReplied(void) const
{
  KMMsgStatus st = status();
  return (st & KMMsgStatusReplied);
}

//-----------------------------------------------------------------------------
bool KMMsgBase::isForwarded(void) const
{
  KMMsgStatus st = status();
  return (st & KMMsgStatusForwarded);
}

//-----------------------------------------------------------------------------
bool KMMsgBase::isQueued(void) const
{
  KMMsgStatus st = status();
  return (st & KMMsgStatusQueued);
}

//-----------------------------------------------------------------------------
bool KMMsgBase::isTodo(void) const
{
  KMMsgStatus st = status();
  return (st & KMMsgStatusTodo);
}

//-----------------------------------------------------------------------------
bool KMMsgBase::isSent(void) const
{
  KMMsgStatus st = status();
  return (st & KMMsgStatusSent);
}

//-----------------------------------------------------------------------------
bool KMMsgBase::isImportant(void) const
{
  KMMsgStatus st = status();
  return (st & KMMsgStatusFlag);
}

//-----------------------------------------------------------------------------
bool KMMsgBase::isWatched(void) const
{
  KMMsgStatus st = status();
  return (st & KMMsgStatusWatched);
}

//-----------------------------------------------------------------------------
bool KMMsgBase::isIgnored(void) const
{
  KMMsgStatus st = status();
  return (st & KMMsgStatusIgnored);
}

//-----------------------------------------------------------------------------
bool KMMsgBase::isSpam(void) const
{
  KMMsgStatus st = status();
  return (st & KMMsgStatusSpam);
}

//-----------------------------------------------------------------------------
bool KMMsgBase::isHam(void) const
{
  KMMsgStatus st = status();
  return (st & KMMsgStatusHam);
}

//-----------------------------------------------------------------------------
QCString KMMsgBase::statusToStr(const KMMsgStatus status)
{
  QCString sstr;
  if (status & KMMsgStatusNew) sstr += 'N';
  if (status & KMMsgStatusUnread) sstr += 'U';
  if (status & KMMsgStatusOld) sstr += 'O';
  if (status & KMMsgStatusRead) sstr += 'R';
  if (status & KMMsgStatusDeleted) sstr += 'D';
  if (status & KMMsgStatusReplied) sstr += 'A';
  if (status & KMMsgStatusForwarded) sstr += 'F';
  if (status & KMMsgStatusQueued) sstr += 'Q';
  if (status & KMMsgStatusTodo) sstr += 'K';
  if (status & KMMsgStatusSent) sstr += 'S';
  if (status & KMMsgStatusFlag) sstr += 'G';
  if (status & KMMsgStatusWatched) sstr += 'W';
  if (status & KMMsgStatusIgnored) sstr += 'I';
  if (status & KMMsgStatusSpam) sstr += 'P';
  if (status & KMMsgStatusHam) sstr += 'H';
  if (status & KMMsgStatusHasAttach) sstr += 'T';
  if (status & KMMsgStatusHasNoAttach) sstr += 'C';

  return sstr;
}

//-----------------------------------------------------------------------------
QString KMMsgBase::statusToSortRank()
{
  QString sstr = "bcbbbbbbbb";

  // put watched ones first, then normal ones, ignored ones last
  if (status() & KMMsgStatusWatched) sstr[0] = 'a';
  if (status() & KMMsgStatusIgnored) sstr[0] = 'c';

  // Second level. One of new, old, read, unread
  if (status() & KMMsgStatusNew) sstr[1] = 'a';
  if (status() & KMMsgStatusUnread) sstr[1] = 'b';
  //if (status() & KMMsgStatusOld) sstr[1] = 'c';
  //if (status() & KMMsgStatusRead) sstr[1] = 'c';

  // Third level. In somewhat arbitrary order.
  if (status() & KMMsgStatusDeleted) sstr[2] = 'a';
  if (status() & KMMsgStatusFlag) sstr[3] = 'a';
  if (status() & KMMsgStatusReplied) sstr[4] = 'a';
  if (status() & KMMsgStatusForwarded) sstr[5] = 'a';
  if (status() & KMMsgStatusQueued) sstr[6] = 'a';
  if (status() & KMMsgStatusSent) sstr[7] = 'a';
  if (status() & KMMsgStatusHam) sstr[8] = 'a';
  if (status() & KMMsgStatusSpam) sstr[8] = 'c';
  if (status() & KMMsgStatusTodo) sstr[9] = 'a';

  return sstr;
}


//-----------------------------------------------------------------------------
void KMMsgBase::setDate(const QCString& aDateStr)
{
  setDate( KRFCDate::parseDate( aDateStr ) );
}


//-----------------------------------------------------------------------------
QString KMMsgBase::dateStr(void) const
{
  time_t d = date();
  return KMime::DateFormatter::formatDate(KMime::DateFormatter::Fancy, d);
}


//-----------------------------------------------------------------------------
QString KMMsgBase::skipKeyword(const QString& aStr, QChar sepChar,
			       bool* hasKeyword)
{
  unsigned int i = 0, maxChars = 3;
  QString str = aStr;

  while (str[0] == ' ') str.remove(0,1);
  if (hasKeyword) *hasKeyword=FALSE;

  unsigned int strLength(str.length());
  for (i=0; i < strLength && i < maxChars; i++)
  {
    if (str[i] < 'A' || str[i] == sepChar) break;
  }

  if (str[i] == sepChar) // skip following spaces too
  {
    do {
      i++;
    } while (str[i] == ' ');
    if (hasKeyword) *hasKeyword=TRUE;
    return str.mid(i);
  }
  return str;
}


//-----------------------------------------------------------------------------
const QTextCodec* KMMsgBase::codecForName(const QCString& _str)
{
  if (_str.isEmpty()) return 0;
  return KGlobal::charsets()->codecForName(_str.lower());
}


//-----------------------------------------------------------------------------
QCString KMMsgBase::toUsAscii(const QString& _str, bool *ok)
{
  bool all_ok =true;
  QString result = _str;
  int len = result.length();
  for (int i = 0; i < len; i++)
    if (result.at(i).unicode() >= 128) {
      result.at(i) = '?';
      all_ok = false;
    }
  if (ok)
    *ok = all_ok;
  return result.latin1();
}


//-----------------------------------------------------------------------------
QStringList KMMsgBase::supportedEncodings(bool usAscii)
{
  QStringList encodingNames = KGlobal::charsets()->availableEncodingNames();
  QStringList encodings;
  QMap<QString,bool> mimeNames;
  for (QStringList::Iterator it = encodingNames.begin();
    it != encodingNames.end(); it++)
  {
    QTextCodec *codec = KGlobal::charsets()->codecForName(*it);
    QString mimeName = (codec) ? QString(codec->mimeName()).lower() : (*it);
    if (mimeNames.find(mimeName) == mimeNames.end())
    {
      encodings.append(KGlobal::charsets()->languageForEncoding(*it)
        + " ( " + mimeName + " )");
      mimeNames.insert(mimeName, TRUE);
    }
  }
  encodings.sort();
  if (usAscii) encodings.prepend(KGlobal::charsets()
    ->languageForEncoding("us-ascii") + " ( us-ascii )");
  return encodings;
}

namespace {
  // don't rely on isblank(), which is a GNU extension in
  // <cctype>. But if someone wants to write a configure test for
  // isblank(), we can then rename this function to isblank and #ifdef
  // it's definition...
  inline bool isBlank( char ch ) { return ch == ' ' || ch == '\t' ; }

  QCString unfold( const QCString & header ) {
    if ( header.isEmpty() )
      return QCString();

    QCString result( header.size() ); // size() >= length()+1 and size() is O(1)
    char * d = result.data();

    for ( const char * s = header.data() ; *s ; )
      if ( *s == '\r' ) { // ignore
	++s;
	continue;
      } else if ( *s == '\n' ) { // unfold
	while ( isBlank( *++s ) );
	*d++ = ' ';
      } else
	*d++ = *s++;

    *d++ = '\0';

    result.truncate( d - result.data() );
    return result;
  }
}


//-----------------------------------------------------------------------------
QString KMMsgBase::decodeRFC2047String(const QCString& aStr)
{
  if ( aStr.isEmpty() )
    return QString::null;

  const QCString str = unfold( aStr );

  if ( str.isEmpty() )
    return QString::null;

  if ( str.find( "=?" ) < 0 )
    return kmkernel->networkCodec()->toUnicode( str );

  QString result;
  QCString LWSP_buffer;
  bool lastWasEncodedWord = false;
  static const int maxLen = 200;

  for ( const char * pos = str.data() ; *pos ; ++pos ) {
    // collect LWSP after encoded-words,
    // because we might need to throw it out
    // (when the next word is an encoded-word)
    if ( lastWasEncodedWord && isBlank( pos[0] ) ) {
      LWSP_buffer += pos[0];
      continue;
    }
    // verbatimly copy normal text
    if (pos[0]!='=' || pos[1]!='?') {
      result += LWSP_buffer + pos[0];
      LWSP_buffer = 0;
      lastWasEncodedWord = FALSE;
      continue;
    }
    // found possible encoded-word
    const char * const beg = pos;
    {
      // parse charset name
      QCString charset;
      int i = 2;
      for (pos+=2; i<maxLen && (*pos!='?'&&(*pos==' '||ispunct(*pos)||isalnum(*pos))); ++i) {
	charset += *pos;
	pos++;
      }
      if (*pos!='?' || i<4 || i>=maxLen)
	goto invalid_encoded_word;

      // get encoding and check delimiting question marks
      const char encoding[2] = { pos[1], '\0' };
      if (pos[2]!='?' || (encoding[0]!='Q' && encoding[0]!='q' &&
			  encoding[0]!='B' && encoding[0]!='b'))
	goto invalid_encoded_word;
      pos+=3; i+=3; // skip ?x?
      const char * enc_start = pos;
      // search for end of encoded part
      while (i<maxLen && *pos && !(*pos=='?' && *(pos+1)=='=')) {
	i++;
	pos++;
      }
      if (i>=maxLen || !*pos)
	goto invalid_encoded_word;

      // valid encoding: decode and throw away separating LWSP
      const KMime::Codec * c = KMime::Codec::codecForName( encoding );
      kdFatal( !c, 5006 ) << "No \"" << encoding << "\" codec!?" << endl;

      QByteArray in; in.setRawData( enc_start, pos - enc_start );
      const QByteArray enc = c->decode( in );
      in.resetRawData( enc_start, pos - enc_start );

      const QTextCodec * codec = codecForName(charset);
      if (!codec) codec = kmkernel->networkCodec();
      result += codec->toUnicode(enc);
      lastWasEncodedWord = true;

      ++pos; // eat '?' (for loop eats '=')
      LWSP_buffer = 0;
    }
    continue;
  invalid_encoded_word:
    // invalid encoding, keep separating LWSP.
    pos = beg;
    result += LWSP_buffer;
    result += "=?";
    lastWasEncodedWord = false;
    ++pos; // eat '?' (for loop eats '=')
    LWSP_buffer = 0;
  }
  return result;
}


//-----------------------------------------------------------------------------
static const QCString especials = "()<>@,;:\"/[]?.= \033";

QCString KMMsgBase::encodeRFC2047Quoted( const QCString & s, bool base64 ) {
  const char * codecName = base64 ? "b" : "q" ;
  const KMime::Codec * codec = KMime::Codec::codecForName( codecName );
  kdFatal( !codec, 5006 ) << "No \"" << codecName << "\" found!?" << endl;
  QByteArray in; in.setRawData( s.data(), s.length() );
  const QByteArray result = codec->encode( in );
  in.resetRawData( s.data(), s.length() );
  return QCString( result.data(), result.size() + 1 );
}

QCString KMMsgBase::encodeRFC2047String(const QString& _str,
  const QCString& charset)
{
  static const QString dontQuote = "\"()<>,@";

  if (_str.isEmpty()) return QCString();
  if (charset == "us-ascii") return toUsAscii(_str);

  QCString cset;
  if (charset.isEmpty()) cset = QCString(kmkernel->networkCodec()->mimeName()).lower();
    else cset = charset;
  const QTextCodec *codec = codecForName(cset);
  if (!codec) codec = kmkernel->networkCodec();

  unsigned int nonAscii = 0;
  unsigned int strLength(_str.length());
  for (unsigned int i = 0; i < strLength; i++)
    if (_str.at(i).unicode() >= 128) nonAscii++;
  bool useBase64 = (nonAscii * 6 > strLength);

  unsigned int start, stop, p, pos = 0, encLength;
  QCString result;
  bool breakLine = FALSE;
  const unsigned int maxLen = 75 - 7 - cset.length();

  while (pos < strLength)
  {
    start = pos; p = pos;
    while (p < strLength)
    {
      if (!breakLine && (_str.at(p) == ' ' || dontQuote.find(_str.at(p)) != -1))
        start = p + 1;
      if (_str.at(p).unicode() >= 128 || _str.at(p).unicode() < 32)
        break;
      p++;
    }
    if (breakLine || p < strLength)
    {
      while (dontQuote.find(_str.at(start)) != -1) start++;
      stop = start;
      while (stop < strLength && dontQuote.find(_str.at(stop)) == -1)
        stop++;
      result += _str.mid(pos, start - pos).latin1();
      encLength = encodeRFC2047Quoted(codec->fromUnicode(_str.
        mid(start, stop - start)), useBase64).length();
      breakLine = (encLength > maxLen);
      if (breakLine)
      {
        int dif = (stop - start) / 2;
        int step = dif;
        while (abs(step) > 1)
        {
          encLength = encodeRFC2047Quoted(codec->fromUnicode(_str.
            mid(start, dif)), useBase64).length();
          step = (encLength > maxLen) ? (-abs(step) / 2) : (abs(step) / 2);
          dif += step;
        }
        stop = start + dif;
      }
      p = stop;
      while (p > start && _str.at(p) != ' ') p--;
      if (p > start) stop = p;
      if (result.right(3) == "?= ") start--;
      if (result.right(5) == "?=\n  ") {
        start--; result.truncate(result.length() - 1);
      }
      int lastNewLine = result.findRev("\n ");
      if (!result.mid(lastNewLine).stripWhiteSpace().isEmpty()
        && result.length() - lastNewLine + encLength + 2 > maxLen)
          result += "\n ";
      result += "=?";
      result += cset;
      result += (useBase64) ? "?b?" : "?q?";
      result += encodeRFC2047Quoted(codec->fromUnicode(_str.mid(start,
        stop - start)), useBase64);
      result += "?=";
      if (breakLine) result += "\n ";
      pos = stop;
    } else {
      result += _str.mid(pos).latin1();
      break;
    }
  }
  return result;
}


//-----------------------------------------------------------------------------
QCString KMMsgBase::encodeRFC2231String( const QString& _str,
                                         const QCString& charset )
{
  if ( _str.isEmpty() )
    return QCString();

  QCString cset;
  if ( charset.isEmpty() )
    cset = QCString( kmkernel->networkCodec()->mimeName() ).lower();
  else
    cset = charset;
  const QTextCodec *codec = codecForName( cset );
  QCString latin;
  if ( charset == "us-ascii" )
    latin = toUsAscii( _str );
  else if ( codec )
    latin = codec->fromUnicode( _str );
  else
    latin = _str.local8Bit();

  char *l;
  for ( l = latin.data(); *l; ++l ) {
    if ( ( *l & 0xE0 == 0 ) || ( *l & 0x80 ) )
      // *l is control character or 8-bit char
      break;
  }
  if ( !*l )
    return latin;

  QCString result = cset + "''";
  for ( l = latin.data(); *l; ++l ) {
    bool needsQuoting = ( *l & 0x80 );
    if( !needsQuoting ) {
      int len = especials.length();
      for ( int i = 0; i < len; i++ )
        if ( *l == especials[i] ) {
          needsQuoting = true;
          break;
        }
    }
    if ( needsQuoting ) {
      result += '%';
      unsigned char hexcode;
      hexcode = ( ( *l & 0xF0 ) >> 4 ) + 48;
      if ( hexcode >= 58 )
        hexcode += 7;
      result += hexcode;
      hexcode = ( *l & 0x0F ) + 48;
      if ( hexcode >= 58 )
        hexcode += 7;
      result += hexcode;
    } else {
      result += *l;
    }
  }
  return result;
}


//-----------------------------------------------------------------------------
QString KMMsgBase::decodeRFC2231String(const QCString& _str)
{
  int p = _str.find('\'');
  if (p < 0) return kmkernel->networkCodec()->toUnicode(_str);

  QCString charset = _str.left(p);

  QCString st = _str.mid(_str.findRev('\'') + 1);
  char ch, ch2;
  p = 0;
  while (p < (int)st.length())
  {
    if (st.at(p) == 37)
    {
      ch = st.at(p+1) - 48;
      if (ch > 16) ch -= 7;
      ch2 = st.at(p+2) - 48;
      if (ch2 > 16) ch2 -= 7;
      st.at(p) = ch * 16 + ch2;
      st.remove( p+1, 2 );
    }
    p++;
  }
  QString result;
  const QTextCodec * codec = codecForName( charset );
  if ( !codec )
    codec = kmkernel->networkCodec();
  return codec->toUnicode( st );
}

QString KMMsgBase::base64EncodedMD5( const QString & s, bool utf8 ) {
  if (s.stripWhiteSpace().isEmpty()) return "";
  if ( utf8 )
    return base64EncodedMD5( s.stripWhiteSpace().utf8() ); // QCString overload
  else
    return base64EncodedMD5( s.stripWhiteSpace().latin1() ); // const char * overload
}

QString KMMsgBase::base64EncodedMD5( const QCString & s ) {
  if (s.stripWhiteSpace().isEmpty()) return "";
  return base64EncodedMD5( s.stripWhiteSpace().data() );
}

QString KMMsgBase::base64EncodedMD5( const char * s, int len ) {
  if (!s || !len) return "";
  static const int Base64EncodedMD5Len = 22;
  KMD5 md5( s, len );
  return md5.base64Digest().left( Base64EncodedMD5Len );
}


//-----------------------------------------------------------------------------
QCString KMMsgBase::autoDetectCharset(const QCString &_encoding, const QStringList &encodingList, const QString &text)
{
    QStringList charsets = encodingList;
    if (!_encoding.isEmpty())
    {
       QString currentCharset = QString::fromLatin1(_encoding);
       charsets.remove(currentCharset);
       charsets.prepend(currentCharset);
    }

    QStringList::ConstIterator it = charsets.begin();
    for (; it != charsets.end(); ++it)
    {
       QCString encoding = (*it).latin1();
       if (encoding == "locale")
          encoding = QCString(kmkernel->networkCodec()->mimeName()).lower();
       if (text.isEmpty())
         return encoding;
       if (encoding == "us-ascii") {
         bool ok;
         (void) KMMsgBase::toUsAscii(text, &ok);
         if (ok)
            return encoding;
       }
       else
       {
         const QTextCodec *codec = KMMsgBase::codecForName(encoding);
         if (!codec) {
           kdDebug(5006) << "Auto-Charset: Something is wrong and I can not get a codec. [" << encoding << "]" << endl;
         } else {
           if (codec->canEncode(text))
              return encoding;
         }
       }
    }
    return 0;
}


//-----------------------------------------------------------------------------
unsigned long KMMsgBase::getMsgSerNum() const
{
  unsigned long msn = MessageProperty::serialCache( this );
  if (msn)
    return msn;
  if (mParent) {
    int index = mParent->find((KMMsgBase*)this);
    msn = kmkernel->msgDict()->getMsgSerNum(mParent, index);
    if (msn)
      MessageProperty::setSerialCache( this, msn );
  }
  return msn;
}


//-----------------------------------------------------------------------------
bool KMMsgBase::isComplete()
{
  return MessageProperty::complete( getMsgSerNum() );
}


//-----------------------------------------------------------------------------
void KMMsgBase::setComplete(bool value)
{
  MessageProperty::setComplete( getMsgSerNum(), value );
  if ( value )
    setReadyToShow( true );
}

//-----------------------------------------------------------------------------
bool KMMsgBase::readyToShow()
{
  return MessageProperty::readyToShow( getMsgSerNum() );
}


//-----------------------------------------------------------------------------
void KMMsgBase::setReadyToShow(bool value)
{
  MessageProperty::setReadyToShow( getMsgSerNum(), value );
}


//-----------------------------------------------------------------------------
bool KMMsgBase::transferInProgress()
{
  return MessageProperty::transferInProgress( getMsgSerNum() );
}


//-----------------------------------------------------------------------------
void KMMsgBase::setTransferInProgress(bool value, bool force)
{
  MessageProperty::setTransferInProgress( getMsgSerNum(), value, force );
}


//-----------------------------------------------------------------------------
KMMsgAttachmentState KMMsgBase::attachmentState() const
{
  KMMsgStatus st = status();
  if (st & KMMsgStatusHasAttach)
    return KMMsgHasAttachment;
  else if (st & KMMsgStatusHasNoAttach)
    return KMMsgHasNoAttachment;
  else
    return KMMsgAttachmentUnknown;
}

//-----------------------------------------------------------------------------
static void swapEndian(QString &str)
{
  uint len = str.length();
  str = QDeepCopy<QString>(str);
  QChar *unicode = const_cast<QChar*>( str.unicode() );
  for (uint i = 0; i < len; i++)
    unicode[i] = kmail_swap_16(unicode[i].unicode());
}

//-----------------------------------------------------------------------------
static int g_chunk_length = 0, g_chunk_offset=0;
static uchar *g_chunk = 0;

namespace {
  template < typename T > void copy_from_stream( T & x ) {
    if( g_chunk_offset + int(sizeof(T)) > g_chunk_length ) {
      g_chunk_offset = g_chunk_length;
      kdDebug( 5006 ) << "This should never happen.. "
		      << __FILE__ << ":" << __LINE__ << endl;
      x = 0;
    } else {
      // the memcpy is optimized out by the compiler for the values
      // of sizeof(T) that is called with
      memcpy( &x, g_chunk + g_chunk_offset, sizeof(T) );
      g_chunk_offset += sizeof(T);
    }
  }
}

//-----------------------------------------------------------------------------
QString KMMsgBase::getStringPart(MsgPartType t) const
{
  QString ret;

  g_chunk_offset = 0;
  bool using_mmap = FALSE;
  bool swapByteOrder = storage()->indexSwapByteOrder();
  if (storage()->indexStreamBasePtr()) {
    if (g_chunk)
	free(g_chunk);
    using_mmap = TRUE;
    g_chunk = storage()->indexStreamBasePtr() + mIndexOffset;
    g_chunk_length = mIndexLength;
  } else {
    if(!storage()->mIndexStream)
      return ret;
    if (g_chunk_length < mIndexLength)
	g_chunk = (uchar *)realloc(g_chunk, g_chunk_length = mIndexLength);
    off_t first_off=ftell(storage()->mIndexStream);
    fseek(storage()->mIndexStream, mIndexOffset, SEEK_SET);
    fread( g_chunk, mIndexLength, 1, storage()->mIndexStream);
    fseek(storage()->mIndexStream, first_off, SEEK_SET);
  }

  MsgPartType type;
  Q_UINT16 l;
  while(g_chunk_offset < mIndexLength) {
    Q_UINT32 tmp;
    copy_from_stream(tmp);
    copy_from_stream(l);
    if (swapByteOrder)
    {
       tmp = kmail_swap_32(tmp);
       l = kmail_swap_16(l);
    }
    type = (MsgPartType) tmp;
    if(g_chunk_offset + l > mIndexLength) {
	kdDebug(5006) << "This should never happen.. " << __FILE__ << ":" << __LINE__ << endl;
	break;
    }
    if(type == t) {
        // This works because the QString constructor does a memcpy.
        // Otherwise we would need to be concerned about the alignment.
	if(l)
	    ret = QString((QChar *)(g_chunk + g_chunk_offset), l/2);
	break;
    }
    g_chunk_offset += l;
  }
  if(using_mmap) {
      g_chunk_length = 0;
      g_chunk = 0;
  }
  // Normally we need to swap the byte order because the QStrings are written
  // in the style of Qt2 (MSB -> network ordered).
  // QStrings in Qt3 expect host ordering.
  // On e.g. Intel host ordering is LSB, on e.g. Sparc it is MSB.

#ifndef WORDS_BIGENDIAN
  // #warning Byte order is little endian (swap is true)
  swapEndian(ret);
#else
  // #warning Byte order is big endian (swap is false)
#endif

  return ret;
}

//-----------------------------------------------------------------------------
off_t KMMsgBase::getLongPart(MsgPartType t) const
{
  off_t ret = 0;

  g_chunk_offset = 0;
  bool using_mmap = FALSE;
  int sizeOfLong = storage()->indexSizeOfLong();
  bool swapByteOrder = storage()->indexSwapByteOrder();
  if (storage()->indexStreamBasePtr()) {
    if (g_chunk)
      free(g_chunk);
    using_mmap = TRUE;
    g_chunk = storage()->indexStreamBasePtr() + mIndexOffset;
    g_chunk_length = mIndexLength;
  } else {
    if (!storage()->mIndexStream)
      return ret;
    assert(mIndexLength >= 0);
    if (g_chunk_length < mIndexLength)
      g_chunk = (uchar *)realloc(g_chunk, g_chunk_length = mIndexLength);
    off_t first_off=ftell(storage()->mIndexStream);
    fseek(storage()->mIndexStream, mIndexOffset, SEEK_SET);
    fread( g_chunk, mIndexLength, 1, storage()->mIndexStream);
    fseek(storage()->mIndexStream, first_off, SEEK_SET);
  }

  MsgPartType type;
  Q_UINT16 l;
  while (g_chunk_offset < mIndexLength) {
    Q_UINT32 tmp;
    copy_from_stream(tmp);
    copy_from_stream(l);
    if (swapByteOrder)
    {
       tmp = kmail_swap_32(tmp);
       l = kmail_swap_16(l);
    }
    type = (MsgPartType) tmp;

    if (g_chunk_offset + l > mIndexLength) {
      kdDebug(5006) << "This should never happen.. " << __FILE__ << ":" << __LINE__ << endl;
      break;
    }
    if(type == t) {
      assert(sizeOfLong == l);
      if (sizeOfLong == sizeof(ret))
      {
	 copy_from_stream(ret);
         if (swapByteOrder)
         {
            if (sizeof(ret) == 4)
               ret = kmail_swap_32(ret);
            else
               ret = kmail_swap_64(ret);
         }
      }
      else if (sizeOfLong == 4)
      {
         // Long is stored as 4 bytes in index file, sizeof(long) = 8
         Q_UINT32 ret_32;
         copy_from_stream(ret_32);
         if (swapByteOrder)
            ret_32 = kmail_swap_32(ret_32);
         ret = ret_32;
      }
      else if (sizeOfLong == 8)
      {
         // Long is stored as 8 bytes in index file, sizeof(long) = 4
         Q_UINT32 ret_1;
         Q_UINT32 ret_2;
         copy_from_stream(ret_1);
         copy_from_stream(ret_2);
         if (!swapByteOrder)
         {
            // Index file order is the same as the order of this CPU.
#ifndef WORDS_BIGENDIAN
            // Index file order is little endian
            ret = ret_1; // We drop the 4 most significant bytes
#else
            // Index file order is big endian
            ret = ret_2; // We drop the 4 most significant bytes
#endif
         }
         else
         {
            // Index file order is different from this CPU.
#ifndef WORDS_BIGENDIAN
            // Index file order is big endian
            ret = ret_2; // We drop the 4 most significant bytes
#else
            // Index file order is little endian
            ret = ret_1; // We drop the 4 most significant bytes
#endif
            // We swap the result to host order.
            ret = kmail_swap_32(ret);
         }

      }
      break;
    }
    g_chunk_offset += l;
  }
  if(using_mmap) {
    g_chunk_length = 0;
    g_chunk = 0;
  }
  return ret;
}

#ifndef WORDS_BIGENDIAN
// We need to use swab to swap bytes to network byte order
#define memcpy_networkorder(to, from, len)  swab((char *)(from), (char *)(to), len)
#else
// We're already in network byte order
#define memcpy_networkorder(to, from, len)  memcpy(to, from, len)
#endif

#define STORE_DATA_LEN(type, x, len, network_order) do { \
	int len2 = (len > 256) ? 256 : len; \
	if(csize < (length + (len2 + sizeof(short) + sizeof(MsgPartType)))) \
    	   ret = (uchar *)realloc(ret, csize += len2+sizeof(short)+sizeof(MsgPartType)); \
        Q_UINT32 t = (Q_UINT32) type; memcpy(ret+length, &t, sizeof(t)); \
        Q_UINT16 l = len2; memcpy(ret+length+sizeof(t), &l, sizeof(l)); \
        if (network_order) \
           memcpy_networkorder(ret+length+sizeof(t)+sizeof(l), x, len2); \
        else \
           memcpy(ret+length+sizeof(t)+sizeof(l), x, len2); \
        length += len2+sizeof(t)+sizeof(l); \
    } while(0)
#define STORE_DATA(type, x) STORE_DATA_LEN(type, &x, sizeof(x), false)

//-----------------------------------------------------------------------------
const uchar *KMMsgBase::asIndexString(int &length) const
{
  unsigned int csize = 256;
  static uchar *ret = 0; //different static buffer here for we may use the other buffer in the functions below
  if(!ret)
    ret = (uchar *)malloc(csize);
  length = 0;

  unsigned long tmp;
  QString tmp_str;

  //these is at the beginning because it is queried quite often
  tmp_str = msgIdMD5().stripWhiteSpace();
  STORE_DATA_LEN(MsgIdMD5Part, tmp_str.unicode(), tmp_str.length() * 2, true);
  tmp = mLegacyStatus;
  STORE_DATA(MsgLegacyStatusPart, tmp);

  //these are completely arbitrary order
  tmp_str = fromStrip().stripWhiteSpace();
  STORE_DATA_LEN(MsgFromPart, tmp_str.unicode(), tmp_str.length() * 2, true);
  tmp_str = subject().stripWhiteSpace();
  STORE_DATA_LEN(MsgSubjectPart, tmp_str.unicode(), tmp_str.length() * 2, true);
  tmp_str = toStrip().stripWhiteSpace();
  STORE_DATA_LEN(MsgToPart, tmp_str.unicode(), tmp_str.length() * 2, true);
  tmp_str = replyToIdMD5().stripWhiteSpace();
  STORE_DATA_LEN(MsgReplyToIdMD5Part, tmp_str.unicode(), tmp_str.length() * 2, true);
  tmp_str = xmark().stripWhiteSpace();
  STORE_DATA_LEN(MsgXMarkPart, tmp_str.unicode(), tmp_str.length() * 2, true);
  tmp_str = fileName().stripWhiteSpace();
  STORE_DATA_LEN(MsgFilePart, tmp_str.unicode(), tmp_str.length() * 2, true);
  tmp = msgSize();
  STORE_DATA(MsgSizePart, tmp);
  tmp = folderOffset();
  STORE_DATA(MsgOffsetPart, tmp);
  tmp = date();
  STORE_DATA(MsgDatePart, tmp);
  tmp = (signatureState() << 16) | encryptionState();
  STORE_DATA(MsgCryptoStatePart, tmp);
  tmp = mdnSentState();
  STORE_DATA(MsgMDNSentPart, tmp);

  tmp_str = replyToAuxIdMD5().stripWhiteSpace();
  STORE_DATA_LEN(MsgReplyToAuxIdMD5Part, tmp_str.unicode(), tmp_str.length() * 2, true);

  tmp_str = strippedSubjectMD5().stripWhiteSpace();
  STORE_DATA_LEN(MsgStrippedSubjectMD5Part, tmp_str.unicode(), tmp_str.length() * 2, true);

  tmp = status();
  STORE_DATA(MsgStatusPart, tmp);

  tmp = msgSizeServer();
  STORE_DATA(MsgSizeServerPart, tmp);
  tmp = UID();
  STORE_DATA(MsgUIDPart, tmp);

  return ret;
}
#undef STORE_DATA_LEN
#undef STORE_DATA

bool KMMsgBase::syncIndexString() const
{
  if(!dirty())
    return TRUE;
  int len;
  const uchar *buffer = asIndexString(len);
  if (len == mIndexLength) {
    Q_ASSERT(storage()->mIndexStream);
    fseek(storage()->mIndexStream, mIndexOffset, SEEK_SET);
    assert( mIndexOffset > 0 );
    fwrite( buffer, len, 1, storage()->mIndexStream);
    return TRUE;
  }
  return FALSE;
}

static QStringList sReplySubjPrefixes, sForwardSubjPrefixes;
static bool sReplaceSubjPrefix, sReplaceForwSubjPrefix;

//-----------------------------------------------------------------------------
void KMMsgBase::readConfig()
{
  KConfigGroup composerGroup( KMKernel::config(), "Composer" );
  sReplySubjPrefixes = composerGroup.readListEntry("reply-prefixes", ',');
  if (sReplySubjPrefixes.isEmpty())
    sReplySubjPrefixes << "Re\\s*:" << "Re\\[\\d+\\]:" << "Re\\d+:";
  sReplaceSubjPrefix = composerGroup.readBoolEntry("replace-reply-prefix", true);
  sForwardSubjPrefixes = composerGroup.readListEntry("forward-prefixes", ',');
  if (sForwardSubjPrefixes.isEmpty())
    sForwardSubjPrefixes << "Fwd:" << "FW:";
  sReplaceForwSubjPrefix = composerGroup.readBoolEntry("replace-forward-prefix", true);
}

//-----------------------------------------------------------------------------
// static
QString KMMsgBase::stripOffPrefixes( const QString& str )
{
  return replacePrefixes( str, sReplySubjPrefixes + sForwardSubjPrefixes,
                          true, QString::null ).stripWhiteSpace();
}

//-----------------------------------------------------------------------------
// static
QString KMMsgBase::replacePrefixes( const QString& str,
                                    const QStringList& prefixRegExps,
                                    bool replace,
                                    const QString& newPrefix )
{
  bool recognized = false;
  // construct a big regexp that
  // 1. is anchored to the beginning of str (sans whitespace)
  // 2. matches at least one of the part regexps in prefixRegExps
  QString bigRegExp = QString::fromLatin1("^(?:\\s+|(?:%1))+\\s*")
                      .arg( prefixRegExps.join(")|(?:") );
  QRegExp rx( bigRegExp, false /*case insens.*/ );
  if ( !rx.isValid() ) {
    kdWarning(5006) << "KMMessage::replacePrefixes(): bigRegExp = \""
                    << bigRegExp << "\"\n"
                    << "prefix regexp is invalid!" << endl;
    // try good ole Re/Fwd:
    recognized = str.startsWith( newPrefix );
  } else { // valid rx
    QString tmp = str;
    if ( rx.search( tmp ) == 0 ) {
      recognized = true;
      if ( replace )
	return tmp.replace( 0, rx.matchedLength(), newPrefix + ' ' );
    }
  }
  if ( !recognized )
    return newPrefix + ' ' + str;
  else
    return str;
}

//-----------------------------------------------------------------------------
QString KMMsgBase::cleanSubject() const
{
  return cleanSubject( sReplySubjPrefixes + sForwardSubjPrefixes,
		       true, QString::null ).stripWhiteSpace();
}

//-----------------------------------------------------------------------------
QString KMMsgBase::cleanSubject( const QStringList & prefixRegExps,
                                 bool replace,
                                 const QString & newPrefix ) const
{
  return KMMsgBase::replacePrefixes( subject(), prefixRegExps, replace,
                                     newPrefix );
}

//-----------------------------------------------------------------------------
QString KMMsgBase::forwardSubject() const {
  return cleanSubject( sForwardSubjPrefixes, sReplaceForwSubjPrefix, "Fwd:" );
}

//-----------------------------------------------------------------------------
QString KMMsgBase::replySubject() const {
  return cleanSubject( sReplySubjPrefixes, sReplaceSubjPrefix, "Re:" );
}
