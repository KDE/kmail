/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifndef kmmsgbase_h
#define kmmsgbase_h

#include "messagestatus.h"
using KPIM::MessageStatus;

// for large file support flags
#include <sys/types.h>
#include <QString>
#include <time.h>

#ifdef KMAIL_SQLITE_INDEX
#include <sqlite3.h>
#endif

class QByteArray;
class QStringList;
class QTextCodec;
class KMFolder;
class KMFolderIndex;
class KMMessageTagList;

/** Flags for the encryption state. */
typedef enum
{
    KMMsgEncryptionStateUnknown=' ',
    KMMsgNotEncrypted='N',
    KMMsgPartiallyEncrypted='P',
    KMMsgFullyEncrypted='F',
    KMMsgEncryptionProblematic='X'
} KMMsgEncryptionState;

/** Flags for the signature state. */
typedef enum
{
    KMMsgSignatureStateUnknown=' ',
    KMMsgNotSigned='N',
    KMMsgPartiallySigned='P',
    KMMsgFullySigned='F',
    KMMsgSignatureProblematic='X'
} KMMsgSignatureState;

/** Flags for the "MDN sent" state. */
typedef enum
{
    KMMsgMDNStateUnknown = ' ',
    KMMsgMDNNone = 'N',
    KMMsgMDNIgnore = 'I',
    KMMsgMDNDisplayed = 'R',
    KMMsgMDNDeleted = 'D',
    KMMsgMDNDispatched = 'F',
    KMMsgMDNProcessed = 'P',
    KMMsgMDNDenied = 'X',
    KMMsgMDNFailed = 'E'
} KMMsgMDNSentState;

/** Flags for the drag and drop action. */
typedef enum
{
    KMMsgDnDActionMOVE=0,
    KMMsgDnDActionCOPY=1,
    KMMsgDnDActionASK=2
} KMMsgDnDAction;

/** Flags for attachment state */
typedef enum
{
  KMMsgHasAttachment,
  KMMsgHasNoAttachment,
  KMMsgAttachmentUnknown
} KMMsgAttachmentState;


class KMMsgBase
{
public:
  KMMsgBase(KMFolder* p=0);

#ifdef KMAIL_SQLITE_INDEX
  KMMsgBase(KMFolder* aParentFolder, char* data, short len, sqlite_int64 id);
#else
  KMMsgBase(KMFolder* aParentFolder, off_t off, short len);
#endif

  virtual ~KMMsgBase();

  /** Return owning storage. */
  KMFolderIndex* storage() const;

  /** Return owning folder. */
  KMFolder* parent() const { return mParent; }

  /** Set owning folder. */
  void setParent(KMFolder* p) { mParent = p; }

  /** Returns true if object is a real message (not KMMsgInfo or KMMsgBase) */
  virtual bool isMessage(void) const;

  /** Status object of the message. */
  virtual MessageStatus& status() const;

  /** Const reference to a status object of a message. */
  const MessageStatus& messageStatus() const;

  /**Get a comma separated list of tag labels*/
  virtual QString tagString( void ) const = 0;
  /**Get a pointer to the tag label list*/
  virtual KMMessageTagList *tagList( void ) const = 0;
  /**Split @p aString at commas and set the tag list to the resulting 
  list and set dirty*/
  virtual void setTagList( const QString &aString );
  /**Set the tag list to @p aTagList and set dirty*/
  virtual void setTagList( const KMMessageTagList &aTagList );

  /** Set status and mark dirty.  Optional optimization: @p idx may
   * specify the index of this message within the parent folder. */
  virtual void setStatus(const MessageStatus& status, int idx = -1);
  virtual void toggleStatus(const MessageStatus& status, int idx = -1);
  virtual void setStatus(const char* statusField, const char* xstatusField=0);

  /** Encryption status of the message. */
  virtual KMMsgEncryptionState encryptionState() const = 0;

  /** Signature status of the message. */
  virtual KMMsgSignatureState signatureState() const = 0;

  /** "MDN send" status of the message. */
  virtual KMMsgMDNSentState mdnSentState() const = 0;

  /** Set "MDN sent" status of the message. */
  virtual void setMDNSentState( KMMsgMDNSentState status, int idx=-1 );

  /** Set encryption status of the message and mark dirty. Optional
   * optimization: @p idx may specify the index of this message within
   * the parent folder. */
  virtual void setEncryptionState(const KMMsgEncryptionState, int idx = -1);

  /** Set signature status of the message and mark dirty. Optional
   * optimization: @p idx may specify the index of this message within
   * the parent folder. */
  virtual void setSignatureState(const KMMsgSignatureState, int idx = -1);

  /** Set encryption status of the message and mark dirty. Optional
   * optimization: @p idx may specify the index of this message within
   * the parent folder. */
  virtual void setEncryptionStateChar( QChar status, int idx = -1 );

  /** Set signature status of the message and mark dirty. Optional
   * optimization: @p idx may specify the index of this message within
   * the parent folder. */
  virtual void setSignatureStateChar( QChar status, int idx = -1 );

  /** Important header fields of the message that are also kept in the index. */
  virtual QString subject(void) const = 0;
  virtual QString fromStrip(void) const = 0;
  virtual QString toStrip(void) const = 0;
  virtual QString replyToIdMD5(void) const = 0;
  virtual QString msgIdMD5(void) const = 0;
  virtual QString replyToAuxIdMD5() const = 0;
  virtual QString strippedSubjectMD5() const = 0;
  virtual bool subjectIsPrefixed() const = 0;
  virtual time_t date(void) const = 0;
  virtual QString dateStr(void) const;
  virtual QString xmark(void) const = 0;

  /** Set date. */
  virtual void setDate(const QByteArray &aStrDate);
  virtual void setDate(time_t aUnixTime) = 0;

  /** Returns true if changed since last folder-sync. */
  virtual bool dirty(void) const { return mDirty; }

  /** Change dirty flag. */
  void setDirty(bool b) { mDirty = b; }

  /** Set subject/from/date and xmark. */
  virtual void setSubject(const QString&) = 0;
  virtual void setXMark(const QString&) = 0;

  /** Calculate strippedSubject */
  virtual void initStrippedSubjectMD5() = 0;

  /** Return contents as index string. This string is of indexStringLength() size */
  const uchar *asIndexString(int &len) const;

  /** Get/set offset in mail folder. */
  virtual off_t folderOffset(void) const = 0;
  virtual void setFolderOffset(off_t offs) = 0;

  /** Get/set msg filename */
  virtual QString fileName(void) const = 0;
  virtual void setFileName(const QString& filename) = 0;

  /** Get/set size of message including the whole header in bytes. */
  virtual size_t msgSize(void) const = 0;
  virtual void setMsgSize(size_t sz) = 0;

  /** Get/set size of message on server */
  virtual size_t msgSizeServer(void) const = 0;
  virtual void setMsgSizeServer(size_t sz) = 0;

  /** Get/set UID for IMAP */
  virtual ulong UID(void) const = 0;
  virtual void setUID(ulong uid) = 0;


#ifdef KMAIL_SQLITE_INDEX
  /** data */
  virtual void setData(char* data) { mData = data; }
  virtual const char* data() const { return mData; }
  /** id of the message (unique within the database) */
  virtual void setDbId(sqlite_int64 dbId) { mDbId = dbId; }
  virtual sqlite_int64 dbId() const { return mDbId; }
#else
  /** offset into index file */
  virtual void setIndexOffset(off_t off) { mIndexOffset = off; }
  virtual off_t indexOffset() const { return mIndexOffset; }
#endif

  /** size in index file (or length of the data() in case of SQLite storage) */
  virtual void setIndexLength(short len) { mIndexLength = len; }
  virtual short indexLength() const { return mIndexLength; }

  /** Skip leading keyword if keyword has given character at it's end
   * (e.g. ':' or ',') and skip the then following blanks (if any) too.
   * If keywordFound is specified it will be true if a keyword was skipped
   * and false otherwise. */
  static QString skipKeyword(const QString& str, QChar sepChar=':',
				 bool* keywordFound=0);

  /** Return a QTextCodec for the specified charset.
   * This function is a bit more tolerant, than QTextCodec::codecForName */
  static const QTextCodec* codecForName(const QByteArray& _str);

  /** Convert all non-ascii characters to question marks
    * If ok is non-null, *ok will be set to true if all characters
    * where ascii, *ok will be set to false otherwise */
  static QByteArray toUsAscii(const QString& _str, bool *ok=0);

  /**
   * Return a list of the supported encodings
   * @param usAscii if true, US-Ascii encoding will be prepended to the list.
   */
  static QStringList supportedEncodings( bool usAscii );

  /**
   * Fixes an encoding received by a KDE function and returns the proper,
   * MIME-compilant encoding name instead.
   * @see encodingForName
   */
  static QString fixEncoding( const QString &encoding );

  /**
   * Drop-in replacement for KCharsets::encodingForName(). The problem with
   * the KCharsets function is that it returns "human-readable" encoding names
   * like "ISO 8859-15" instead of valid encoding names like "ISO-8859-15".
   * This function fixes this by replacing whitespace with a hyphen.
   */
  static QString encodingForName( const QString &descriptiveName );

  /** Copy all values from other to this object. */
  void assign(const KMMsgBase* other);

  /** Assignment operator that simply calls assign(). */
  KMMsgBase& operator=(const KMMsgBase& other);

  /** Copy constructor that simply calls assign(). */
  KMMsgBase( const KMMsgBase &other );

  /** Helper function for encodeRFC2047String */
  static QByteArray encodeRFC2047Quoted( const QByteArray &aStr, bool base64 );

  /** This function handles both encodings described in RFC2047:
    Base64 ("=?iso-8859-1?b?...?=") and quoted-printable */
  static QString decodeRFC2047String( const QByteArray &aStr,
                                      const QByteArray &prefCharset = "" );

  /** Encode given string as described in RFC2047:
    using quoted-printable. */
  static QByteArray encodeRFC2047String( const QString &aStr,
                                         const QByteArray &charset );

  /** Encode given string as described in RFC2231
    (parameters in MIME headers) */
  static QByteArray encodeRFC2231String( const QString &aStr,
                                         const QByteArray &charset );

  /** Decode given string as described in RFC2231 */
  static QString decodeRFC2231String( const QByteArray &aStr );

  /** Extract a given param from the RFC2231-encoded header field, in
      particular concatenate possibly multiple entries, which are given as
      paramname*0=..; paramname*1=..; ... or paramname*0*=..; paramname*1*=..;
      ... and return their value as one string. That string will still be
      encoded
  */
  static QByteArray extractRFC2231HeaderField( const QByteArray &aStr,
                                               const QByteArray &field );


  /** Calculate the base64 encoded md5sum (sans the trailing equal
      signs). If @p utf8 is false, uses QString::toLatin1() to calculate
      the md5sum of, else uses QString::utf8() */
  static QString base64EncodedMD5( const QString & aStr, bool utf8=false );
  static QString base64EncodedMD5( const QByteArray & aStr );
  static QString base64EncodedMD5( const char * aStr, int len=-1 );

  /**
   * Find out preferred charset for 'text'.
   * First @p encoding is tried and if that one is not suitable,
   * the encodings in @p encodingList are tried.
   */
  static QByteArray autoDetectCharset(const QByteArray &encoding, const QStringList &encodingList, const QString &text);

  /** Returns the message serial number for the message. */
  virtual unsigned long getMsgSerNum() const;

  /** If undo for this message should be enabled */
  virtual bool enableUndo() { return mEnableUndo; }
  virtual void setEnableUndo( bool enable ) { mEnableUndo = enable; }

  /** Return if the message has at least one attachment */
  virtual KMMsgAttachmentState attachmentState() const;

  /** Check for prefixes @p prefixRegExps in @p str. If none
      is found, @p newPrefix + ' ' is prepended to @p str and the
      resulting string is returned. If @p replace is true, any
      sequence of whitespace-delimited prefixes at the beginning of
      @p str is replaced by @p newPrefix.
  **/
  static QString replacePrefixes( const QString& str,
                                  const QStringList& prefixRegExps,
                                  bool replace,
                                  const QString& newPrefix );

  /** Returns @p str with all "forward" and "reply" prefixes stripped off.
   **/
  static QString stripOffPrefixes( const QString& str );

  /** Check for prefixes @p prefixRegExps in #subject(). If none
      is found, @p newPrefix + ' ' is prepended to the subject and the
      resulting string is returned. If @p replace is true, any
      sequence of whitespace-delimited prefixes at the beginning of
      #subject() is replaced by @p newPrefix
  **/
  QString cleanSubject( const QStringList& prefixRegExps, bool replace,
                        const QString& newPrefix ) const;

  /** Return this mails subject, with all "forward" and "reply"
      prefixes removed */
  QString cleanSubject() const;

  /** Return this mails subject, formatted for "forward" mails */
  QString forwardSubject() const;

  /** Return this mails subject, formatted for "reply" mails */
  QString replySubject() const;

  /** Reads config settings from group "Composer" and sets all internal
   * variables (e.g. indent-prefix, etc.) */
  static void readConfig();

protected:
  KMFolder* mParent;
#ifdef KMAIL_SQLITE_INDEX
  char* mData;
  sqlite_int64 mDbId; //!< id of the message (unique within the database)
#else
  off_t mIndexOffset;
#endif
  short mIndexLength;
  bool mDirty;
  bool mEnableUndo;
  mutable MessageStatus mStatus;
  mutable KMMessageTagList *mTagList;

public:
  enum MsgPartType
  {
    MsgNoPart = 0,
    //unicode strings
    MsgFromPart = 1,
    MsgSubjectPart = 2,
    MsgToPart = 3,
    MsgReplyToIdMD5Part = 4,
    MsgIdMD5Part = 5,
    MsgXMarkPart = 6,
    //unsigned long
    MsgOffsetPart = 7,
    MsgLegacyStatusPart = 8,
    MsgSizePart = 9,
    MsgDatePart = 10,
    MsgFilePart = 11,
    MsgCryptoStatePart = 12,
    MsgMDNSentPart = 13,
    //another two unicode strings
    MsgReplyToAuxIdMD5Part = 14,
    MsgStrippedSubjectMD5Part = 15,
    // and another unsigned long
    MsgStatusPart = 16,
    MsgSizeServerPart = 17,
    MsgUIDPart = 18,
    // unicode string
    MsgTagPart = 19
  };
  /** access to long msgparts */
  off_t getLongPart(MsgPartType) const;
  /** access to string msgparts */
  QString getStringPart(MsgPartType) const;
#ifndef KMAIL_SQLITE_INDEX
  /** sync'ing just one KMMsgBase */
  bool syncIndexString() const;
#endif
};

#endif /*kmmsgbase_h*/
