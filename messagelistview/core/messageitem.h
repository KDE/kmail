/******************************************************************************
 *
 *  Copyright 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_MESSAGEITEM_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_MESSAGEITEM_H__

#include "messagelistview/core/item.h"
#include "messagelistview/core/modelinvariantindex.h"

#include <QPixmap>
#include <QString>
#include <QColor>

namespace KMail
{

namespace MessageListView
{

namespace Core
{

class MessageItem : public Item, public ModelInvariantIndex
{
  friend class Model;
  friend class SkinPreviewDelegate;

public:
  class Tag
  {
  protected:
    QPixmap mPixmap;      ///< The pixmap associated to this tag
    QString mName;        ///< The name of this tag
    QString mId;          ///< The unique id of this tag
  public:
    Tag( const QPixmap &pix, const QString &tagName, const QString &tagId )
      : mPixmap( pix ), mName( tagName ), mId( tagId )
      {};
    const QPixmap & pixmap() const
      { return mPixmap; };
    const QString & name() const
      { return mName; };
    const QString & id() const
      { return mId; };
  };

  enum ThreadingStatus
  {
    PerfectParentFound,     ///< this message found a perfect parent to attach to
    ImperfectParentFound,   ///< this message found an imperfect parent to attach to (might be fixed later)
    ParentMissing,          ///< this message might belong to a thread but its parent is actually missing
    NonThreadable           ///< this message does not look as being threadable
  };

  enum EncryptionState
  {
    NotEncrypted,
    PartiallyEncrypted,
    FullyEncrypted,
    EncryptionStateUnknown
  };

  enum SignatureState
  {
    NotSigned,
    PartiallySigned,
    FullySigned,
    SignatureStateUnknown
  };

protected:
  MessageItem();
  virtual ~MessageItem();


private:
  ThreadingStatus mThreadingStatus;
  QString mMessageIdMD5;            ///< always set
  QString mInReplyToIdMD5;          ///< set only if we're doing threading
  QString mReferencesIdMD5;         ///< set only if we're doing threading
  QString mStrippedSubjectMD5;      ///< set only if we're doing threading
  bool mSubjectIsPrefixed;          ///< set only if we're doing subject based threading
  EncryptionState mEncryptionState;
  SignatureState mSignatureState;
  QList< Tag * > * mTagList;        ///< Usually 0....
  QColor mTextColor;                ///< If invalid, use default text color
  unsigned long mUniqueId;          ///< The unique id of this message (serial number of KMMsgBase at the moment of writing)
  
  bool mAboutToBeRemoved;           ///< Set to true when this item is going to be deleted and shouldn't be selectable
public:
  QList< Tag * > * tagList() const
    { return mTagList; };

  void setTagList( QList< Tag * > * list );

  const QColor & textColor() const
    { return mTextColor; };

  void setTextColor( const QColor &clr )
    { mTextColor = clr; };

  SignatureState signatureState() const
    { return mSignatureState; };

  void setSignatureState( SignatureState state )
    { mSignatureState = state; };

  EncryptionState encryptionState() const
    { return mEncryptionState; };

  void setEncryptionState( EncryptionState state )
    { mEncryptionState = state; };

  const QString & messageIdMD5() const
    { return mMessageIdMD5; };

  void setMessageIdMD5( const QString &md5 )
    { mMessageIdMD5 = md5; };

  const QString & inReplyToIdMD5() const
    { return mInReplyToIdMD5; };

  void setInReplyToIdMD5( const QString &md5 )
    { mInReplyToIdMD5 = md5; };

  const QString & referencesIdMD5() const
    { return mReferencesIdMD5; };

  void setReferencesIdMD5( const QString &md5 )
    { mReferencesIdMD5 = md5; };

  void setSubjectIsPrefixed( bool subjectIsPrefixed )
    { mSubjectIsPrefixed = subjectIsPrefixed; };

  bool subjectIsPrefixed() const
    { return mSubjectIsPrefixed; };

  const QString & strippedSubjectMD5() const
    { return mStrippedSubjectMD5; };

  void setStrippedSubjectMD5( const QString &md5 )
    { mStrippedSubjectMD5 = md5; };

  bool aboutToBeRemoved() const
    { return mAboutToBeRemoved; };

  void setAboutToBeRemoved( bool aboutToBeRemoved )
    { mAboutToBeRemoved = aboutToBeRemoved; };

  ThreadingStatus threadingStatus() const
    { return mThreadingStatus; };

  void setThreadingStatus( ThreadingStatus threadingStatus )
    { mThreadingStatus = threadingStatus; };

  unsigned long uniqueId() const
    { return mUniqueId; };

  void setUniqueId(unsigned long uniqueId)
    { mUniqueId = uniqueId; };

  MessageItem * topmostMessage();

  /**
   * Appends the whole subtree originating at this item
   * to the specified list. This item is included!
   */
  void subTreeToList( QList< MessageItem * > &list );
};

} // namespace Core

} // namespace MessageListView

} // namespace KMail

#endif //!__KMAIL_MESSAGELISTVIEW_CORE_MESSAGEITEM_H__
