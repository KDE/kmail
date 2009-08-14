/* -*- c++ -*-
    partNode.h A node in a MIME tree.

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002,2004 Klarälvdalens Datakonsult AB

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef PARTNODE_H
#define PARTNODE_H

#include "kmmsgpart.h"
#include "kmmsgbase.h"
#include "kmmessage.h"

#include "interfaces/bodypart.h"

#include <mimelib/mimepp.h>
#include <mimelib/body.h>
#include <mimelib/utility.h>

#include <kio/global.h>
#include <kdebug.h>
#include <QByteArray>

#include <map>

class KMMimePartTreeItem;
class KMMimePartTree;

class KMReaderWin;

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
    partNode();

    int calcNodeIdOrFindNode( int& curId, const partNode* calcNode,
                              int findId, partNode** findNode );

    partNode( KMReaderWin *win, DwBodyPart *dwPart,
              int explicitType    = DwMime::kTypeUnknown,
              int explicitSubType = DwMime::kSubtypeUnknown,
	      bool deleteDwBodyPart = false );

public:
    static partNode *fromMessage( const KMMessage *msg, KMReaderWin *win=0 );

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

    const QByteArray & encodedBody();

    void setType( int type ) {
        mType = type;
    }

    void setSubType( int subType ) {
        mSubType = subType;
    }

    int type() const {
        return mType;
    }

    QByteArray typeString() const;

    int subType() const {
        return mSubType;
    }

    QByteArray subTypeString() const;

    bool hasType( int type ) {
      return mType == type;
    }

    bool hasSubType( int subType ) {
      return mSubType == subType;
    }

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

    // path is a hierarchical path to this partNode. It is designed to
    // be stable under decryption, where new child nodes are
    // added. Treat it as an opaque string.
    QByteArray path() const;

    int nodeId() const;  // node ids start at 1 (this is the top level root node)

    partNode* findId( int id );  // returns the node which has the given id (or 0, resp.)

    partNode* findType( int type, int subType, bool deep=true, bool wide=true );

    partNode* findTypeNot( int type, int subType, bool deep=true,
                           bool wide=true );

    partNode* findNodeForDwPart( DwBodyPart* part );

    void fillMimePartTree( KMMimePartTreeItem* parentItem,
                           KMMimePartTree*     mimePartTree,
                           const QString &labelDescr    = QString(),
                           const QString &labelCntType  = QString(),
                           const QString &labelEncoding = QString(),
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
    bool isHeuristicalAttachment() const;
    /** returns true if this is the first text part of the message this node
        is a body part of
    */
    bool isFirstTextPart() const;

    bool hasContentDispositionInline() const;

    QString contentTypeParameter( const char * name ) const;

    const QString& trueFromAddress() const;

    const partNode * topLevelParent() const;
    partNode * parentNode() const { return mRoot; }
    partNode * nextSibling() const { return mNext; }
    partNode * firstChild() const { return mChild; }
    partNode * next( bool allowChildren=true ) const;
    int childCount() const;
    int totalChildCount() const;
    bool processed() const { return mWasProcessed; }

    KMail::Interface::BodyPartMemento *bodyPartMemento( const QByteArray &which ) const;
    void setBodyPartMemento( const QByteArray &which, KMail::Interface::BodyPartMemento *memento );

    // A flag to remember if the node was embedded. This is useful for attachment nodes, the reader
    // needs to know if they were displayed inline or not.
    bool isDisplayedEmbedded() const;
    void setDisplayedEmbedded( bool displayedEmbedded );

    // Get a href in the form attachment:<nodeId>?place=<place>, used by ObjectTreeParser and
    // UrlHandlerManager.
    QString asHREF( const QString &place ) const;

private:
    KMReaderWin *reader() const {
      return mReader ? mReader : mRoot ? mRoot->reader() : 0 ;
    }
    KMail::Interface::BodyPartMemento *internalBodyPartMemento( const QByteArray & ) const;
    void internalSetBodyPartMemento( const QByteArray &which, KMail::Interface::BodyPartMemento *memento );

private:
    partNode*     mRoot;
    partNode*     mNext;
    partNode*     mChild;
    bool          mWasProcessed; // to be used by parseObjectTree()
private:
    DwBodyPart*   mDwPart;   // may be zero
    mutable KMMessagePart mMsgPart;  // is valid - even if mDwPart is zero
    QByteArray    mEncodedBody;
    QString       mFromAddress;
    int           mType;
    int           mSubType;
    KMMsgEncryptionState mEncryptionState;
    KMMsgSignatureState  mSignatureState;
    mutable bool  mMsgPartOk;
    bool          mEncodedOk;
    bool          mDeleteDwBodyPart;
    KMMimePartTreeItem* mMimePartTreeItem;
    std::map<QByteArray,KMail::Interface::BodyPartMemento*> mBodyPartMementoMap;
    KMReaderWin  *mReader;
    bool mDisplayedEmbedded;
};

#endif
