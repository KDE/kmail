/*
  $Id$

  a node in a MIME tree.

  Copyright (C) 2002 by Klarälvdalens Datakonsult AB

  This code is licensed under the GPL, blah, blah, blah...
*/


#ifndef PARTNODE_H
#define PARTNODE_H

#include "kmmsgpart.h"
#include "kmmsgbase.h"
#include "kmmessage.h"

#include <mimelib/mimepp.h>
#include <mimelib/body.h>
#include <mimelib/utility.h>

#include <kdebug.h>
#include <kio/global.h>

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
    partNode() :
        mRoot(      0 ),
        mPrev(      0 ),
        mNext(      0 ),
        mChild(     0 ),
        mWasProcessed( false ),
        mDwPart(       0 ),
        mType(         DwMime::kTypeUnknown  ),
        mSubType(      DwMime::kSubtypeUnknown ),
        mCryptoType(   CryptoTypeUnknown ),
        mEncryptionState( KMMsgNotEncrypted ),
        mSignatureState(  KMMsgNotSigned ),
        mMsgPartOk(    false ),
        mEncodedOk(    false ),
        mDeleteDwBodyPart( false ),
        mMimePartTreeItem( 0 ) {
        adjustDefaultType( this );
    }

    int calcNodeIdOrFindNode( int& curId, const partNode* calcNode,
                              int findId, partNode** findNode );

public:
    partNode( DwBodyPart* dwPart,
              int explicitType    = DwMime::kTypeUnknown,
              int explicitSubType = DwMime::kSubtypeUnknown,
	      bool deleteDwBodyPart = false ) :
        mRoot(      0 ),
        mPrev(      0 ),
        mNext(      0 ),
        mChild(     0 ),
        mWasProcessed( false ),
        mDwPart(       dwPart ),
        mCryptoType(   CryptoTypeUnknown ),
        mEncryptionState( KMMsgNotEncrypted ),
        mSignatureState(  KMMsgNotSigned ),
        mMsgPartOk(    false ),
        mEncodedOk(    false ),
        mDeleteDwBodyPart( deleteDwBodyPart ),
        mMimePartTreeItem( 0 ) {
        if( explicitType != DwMime::kTypeUnknown ) {
            mType    = explicitType;     // this happens e.g. for the Root Node
            mSubType = explicitSubType;  // representing the _whole_ message
        } else {
            kdDebug(5006) << "\n        partNode::partNode()      explicitType == DwMime::kTypeUnknown\n" << endl;
            if(dwPart && dwPart->hasHeaders() && dwPart->Headers().HasContentType()) {
                mType    = (!dwPart->Headers().ContentType().Type())?DwMime::kTypeUnknown:dwPart->Headers().ContentType().Type();
                mSubType = dwPart->Headers().ContentType().Subtype();
            } else {
                mType    = DwMime::kTypeUnknown;
                mSubType = DwMime::kSubtypeUnknown;
            }
        }
#ifdef DEBUG
        {
            DwString type, subType;
            DwTypeEnumToStr(    mType,    type );
            DwSubtypeEnumToStr( mSubType, subType );
            kdDebug(5006) << "\npartNode::partNode()   " << type.c_str() << "/" << subType.c_str() << "\n" << endl;
        }
#endif
    }

    partNode( bool deleteDwBodyPart,
              DwBodyPart* dwPart ) :
        mRoot(      0 ),
        mPrev(      0 ),
        mNext(      0 ),
        mChild(     0 ),
        mWasProcessed( false ),
        mDwPart(       dwPart ),
        mCryptoType(   CryptoTypeUnknown ),
        mEncryptionState( KMMsgNotEncrypted ),
        mSignatureState(  KMMsgNotSigned ),
        mMsgPartOk(    false ),
        mEncodedOk(    false ),
        mDeleteDwBodyPart( deleteDwBodyPart ),
        mMimePartTreeItem( 0 ) {
        if(dwPart && dwPart->hasHeaders() && dwPart->Headers().HasContentType()) {
            mType    = (!dwPart->Headers().ContentType().Type())?DwMime::kTypeUnknown:dwPart->Headers().ContentType().Type();
            mSubType = dwPart->Headers().ContentType().Subtype();
        } else {
            mType    = DwMime::kTypeUnknown;
            mSubType = DwMime::kSubtypeUnknown;
        }
    }

    ~partNode() {
        // 0. delete the DwBodyPart if flag is set
        if( mDeleteDwBodyPart && mDwPart ) delete( mDwPart );
        // 1. delete our children
        delete( mChild );
        // 2. delete our siblings
        delete( mNext );
    }

    void buildObjectTree( bool processSiblings=true );

    DwBodyPart* dwPart() const {
        return mDwPart;
    }

    KMMessagePart& msgPart() {
        if( !mMsgPartOk ) {
            KMMessage::bodyPart(mDwPart, &mMsgPart);
            mMsgPartOk = true;
        }
        return mMsgPart;
    }

    QCString& encodedBody() {
        if( !mEncodedOk ) {
            if( mDwPart )
                mEncodedBody = mDwPart->AsString().c_str();
            else
                mEncodedBody = "";
            mEncodedOk = true;
        }
        return mEncodedBody;
    }

    void setType( int type ) {
        mType = type;
    }

    void setSubType( int subType ) {
        mSubType = subType;
    }

    int type() const {
        return mType;
    }

    int subType() const {
        return mSubType;
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

    partNode* findId( int id );  // returns the node wich has the given id (or 0, resp.)

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

    partNode* setNext( partNode* next ) {
        mNext = next;
        if( mNext ){
            mNext->mRoot = mRoot;
            mNext->mPrev = this;
            adjustDefaultType( mNext );
        }
        return mNext;
    }

    partNode* setFirstChild( partNode* child ) {
        mChild = child;
        if( mChild ) {
            mChild->mRoot = this;
            adjustDefaultType( mChild );
        }
        return mChild;
    }

    void setProcessed( bool wasProcessed ) {
        mWasProcessed = wasProcessed;
        if( mChild )
            mChild->setProcessed( wasProcessed );
        if( mNext )
            mNext->setProcessed( wasProcessed );
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

    const QString& trueFromAddress() const;

public:
    partNode*     mRoot;
    partNode*     mPrev;
    partNode*     mNext;
    partNode*     mChild;
    bool          mWasProcessed; // to be used by parseObjectTree()
private:
    DwBodyPart*   mDwPart;   // may be zero
    KMMessagePart mMsgPart;  // is valid - even if mDwPart is zero
    QCString      mEncodedBody;
    QString       mFromAddress;
    int           mType;
    int           mSubType;
    CryptoType    mCryptoType;
    KMMsgEncryptionState mEncryptionState;
    KMMsgSignatureState  mSignatureState;
    bool          mMsgPartOk;
    bool          mEncodedOk;
    bool          mDeleteDwBodyPart;
    KMMimePartTreeItem* mMimePartTreeItem;
};

#endif
