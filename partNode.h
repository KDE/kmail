/*
   A node in a MIME tree.

    Copyright (C) 2002 by Klarälvdalens Datakonsult AB

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/


#ifndef PARTNODE_H
#define PARTNODE_H

#include "kmmsgpart.h"
#include "kmmsgbase.h"
#include "kmmessage.h"

#include <mimelib/mimepp.h>
#include <mimelib/body.h>
#include <mimelib/utility.h>

#include <kio/global.h>
#include <kdebug.h>

class KMMimePartTreeItem;
class KMMimePartTree;

/*
 ===========================================================================


       S T A R T    O F     T E M P O R A R Y     M I M E     C O D E


 ===========================================================================
  N O T E :   The partNode structure will most likely be replaced by KMime.
              It's purpose: Speed optimization for KDE 3.   (khz, 28.11.01)
 ===========================================================================
*/
class partNode
{
public:
    enum CryptoType { CryptoTypeUnknown,
                      CryptoTypeNone,
                      CryptoTypeInlinePGP,
                      CryptoTypeOpenPgpMIME,
                      CryptoTypeSMIME,
                      CryptoType3rdParty };

private:
    partNode();

    int calcNodeIdOrFindNode( int& curId, const partNode* calcNode,
                              int findId, partNode** findNode );

public:
    partNode( DwBodyPart* dwPart,
              int explicitType    = DwMime::kTypeUnknown,
              int explicitSubType = DwMime::kSubtypeUnknown,
	      bool deleteDwBodyPart = false );

    partNode( bool deleteDwBodyPart,
              DwBodyPart* dwPart );

    ~partNode();

    void dump( int chars=0 ) const;

    void buildObjectTree( bool processSiblings=true );

    DwBodyPart* dwPart() const {
        return mDwPart;
    }

    void setDwPart( DwBodyPart* part ) {
        mDwPart = part;
        mMsgPartOk = false;
    }

    KMMessagePart& msgPart() const {
        if( !mMsgPartOk ) {
            KMMessage::bodyPart(mDwPart, &mMsgPart);
            mMsgPartOk = true;
        }
        return mMsgPart;
    }

    const QCString & encodedBody();

    void setType( int type ) {
        mType = type;
    }

    void setSubType( int subType ) {
        mSubType = subType;
    }

    int type() const {
        return mType;
    }

    QCString typeString() const;

    int subType() const {
        return mSubType;
    }

    QCString subTypeString() const;

    bool hasType( int type ) {
      return mType == type;
    }

    bool hasSubType( int subType ) {
      return mSubType == subType;
    }

    void setCryptoType( CryptoType cryptoType ) {
        mCryptoType = cryptoType;
    }

    CryptoType cryptoType() const {
        return mCryptoType;
    }

    // return first not-unknown and not-none crypto type
    // or return none (or unknown, resp.) if no other crypto type set
    CryptoType firstCryptoType() const ;

    void setEncryptionState( KMMsgEncryptionState state ) {
        mEncryptionState = state;
    }
    KMMsgEncryptionState encryptionState() const {
        return mEncryptionState;
    }

    // look at the encryption states of all children and return result
    KMMsgEncryptionState overallEncryptionState() const ;

    // look at the signature states of all children and return result
    KMMsgSignatureState  overallSignatureState() const ;

    void setSignatureState( KMMsgSignatureState state ) {
        mSignatureState = state;
    }
    KMMsgSignatureState signatureState() const {
        return mSignatureState;
    }

    int nodeId();  // node ids start at 1 (this is the top level root node)

    partNode* findId( int id );  // returns the node which has the given id (or 0, resp.)

    partNode* findType( int type, int subType, bool deep=true, bool wide=true );

    partNode* findTypeNot( int type, int subType, bool deep=true,
                           bool wide=true );

    void fillMimePartTree( KMMimePartTreeItem* parentItem,
                           KMMimePartTree*     mimePartTree,
                           QString labelDescr    = QString::null,
                           QString labelCntType  = QString::null,
                           QString labelEncoding = QString::null,
                           KIO::filesize_t size=0,
                           bool revertOrder = false );

    void adjustDefaultType( partNode* node );

    void setNext( partNode* next ) {
        mNext = next;
        if( mNext ){
            mNext->mRoot = mRoot;
            adjustDefaultType( mNext );
        }
    }

    void setFirstChild( partNode* child ) {
        mChild = child;
        if( mChild ) {
            mChild->mRoot = this;
            adjustDefaultType( mChild );
        }
    }

    void setProcessed( bool processed, bool recurse ) {
        mWasProcessed = processed;
	if ( recurse ) {
	  if( mChild )
            mChild->setProcessed( processed, true );
	  if( mNext )
            mNext->setProcessed( processed, true );
	}
    }

    void setMimePartTreeItem( KMMimePartTreeItem* item ) {
        mMimePartTreeItem = item;
    }

    KMMimePartTreeItem* mimePartTreeItem() {
        return mMimePartTreeItem;
    }

    void setFromAddress( const QString& address ) {
        mFromAddress = address;
    }

    bool isAttachment() const;

    bool hasContentDispositionInline() const;

    const QString& trueFromAddress() const;

    partNode * parentNode() const { return mRoot; }
    partNode * nextSibling() const { return mNext; }
    partNode * firstChild() const { return mChild; }
    bool processed() const { return mWasProcessed; }

private:
    partNode*     mRoot;
    partNode*     mNext;
    partNode*     mChild;
    bool          mWasProcessed; // to be used by parseObjectTree()
private:
    DwBodyPart*   mDwPart;   // may be zero
    mutable KMMessagePart mMsgPart;  // is valid - even if mDwPart is zero
    QCString      mEncodedBody;
    QString       mFromAddress;
    int           mType;
    int           mSubType;
    CryptoType    mCryptoType;
    KMMsgEncryptionState mEncryptionState;
    KMMsgSignatureState  mSignatureState;
    mutable bool  mMsgPartOk;
    bool          mEncodedOk;
    bool          mDeleteDwBodyPart;
    KMMimePartTreeItem* mMimePartTreeItem;
};

#endif
