/*
  $Id$

  a node in a MIME tree.

  Copyright (C) 2002 by Klar�lvdalens Datakonsult AB

  This code is licensed under the GPL, blah, blah, blah...
*/

#include <config.h>
#include "partNode.h"
#include <klocale.h>
#include <kdebug.h>
#include "kmcomposewin.h"
#include "kmmimeparttree.h"

/*
  ===========================================================================


  S T A R T    O F     T E M P O R A R Y     M I M E     C O D E


  ===========================================================================
  N O T E :   The partNode structure will most likely be replaced by KMime.
  It's purpose: Speed optimization for KDE 3.   (khz, 28.11.01)
  ===========================================================================
*/

void partNode::buildObjectTree( bool processSiblings )
{
    partNode* curNode = this;
    while( curNode && curNode->dwPart() ) {
        //dive into multipart messages
        while( DwMime::kTypeMultipart == curNode->type() ) {
            partNode* newNode = curNode->setFirstChild(
                new partNode( curNode->dwPart()->Body().FirstBodyPart() ) );
            curNode = newNode;
        }
        // go up in the tree until reaching a node with next
        // (or the last top-level node)
        while(     curNode
               && !(    curNode->dwPart()
                     && curNode->dwPart()->Next() ) ) {
            curNode = curNode->mRoot;
        }
        // we might have to leave when all children have been processed
        if( this == curNode && !processSiblings )
            return;
        // store next node
        if( curNode && curNode->dwPart() && curNode->dwPart()->Next() ) {
            partNode* nextNode = new partNode( curNode->dwPart()->Next() );
            curNode = curNode->setNext( nextNode );
        } else
            curNode = 0;
    }
}


KMMsgEncryptionState partNode::overallEncryptionState() const
{
    KMMsgEncryptionState otherState;
    KMMsgEncryptionState myState;
    if( mIsEncrypted )
        myState = KMMsgFullyEncrypted;
    else {
        // NOTE: children are tested ONLY when parent is not encrypted
        if( mChild )
            myState = mChild->overallEncryptionState();
        else
            myState = KMMsgNotEncrypted;
    }
    // siblings are tested allways
    if( mNext )
        otherState = mNext->overallEncryptionState(); {
        switch( otherState ) {
        case KMMsgEncryptionStateUnknown:
            break;
        case KMMsgNotEncrypted:
            if( myState == KMMsgFullyEncrypted )
                myState = KMMsgPartiallyEncrypted;
            else if( myState != KMMsgPartiallyEncrypted )
                myState = KMMsgNotEncrypted;
            break;
        case KMMsgPartiallyEncrypted:
            myState = KMMsgPartiallyEncrypted;
            break;
        case KMMsgFullyEncrypted:
            if( myState != KMMsgFullyEncrypted )
                myState = KMMsgPartiallyEncrypted;
            break;
        }
    }

kdDebug(5006) << "\n\n  KMMsgEncryptionState: " << myState << endl;

    return myState;
}


KMMsgSignatureState  partNode::overallSignatureState() const
{
    KMMsgSignatureState otherState;
    KMMsgSignatureState myState;
    if( mIsSigned )
        myState = KMMsgFullySigned;
    else {
        // children are tested ONLY when parent is not signed
        if( mChild )
            myState = mChild->overallSignatureState();
        else
            myState = KMMsgNotSigned;
    }
    // siblings are tested allways
    if( mNext ) {
        otherState = mNext->overallSignatureState();
        switch( otherState ) {
        case KMMsgSignatureStateUnknown:
            break;
        case KMMsgNotSigned:
            if( myState == KMMsgFullySigned )
                myState = KMMsgPartiallySigned;
            else if( myState != KMMsgPartiallySigned )
                myState = KMMsgNotSigned;
            break;
        case KMMsgPartiallySigned:
            myState = KMMsgPartiallySigned;
            break;
        case KMMsgFullySigned:
            if( myState != KMMsgFullySigned )
                myState = KMMsgPartiallySigned;
            break;
        }
    }

kdDebug(5006) << "\n\n  KMMsgSignatureState: " << myState << endl;

    return myState;
}


int partNode::nodeId()
{
    partNode* rootNode = this;
    while( rootNode->mRoot )
        rootNode = rootNode->mRoot;
    return rootNode->calcNodeIdOrFindNode( 0, this, 0, 0 );
}


partNode* partNode::findId( int id )
{
    partNode* rootNode = this;
    while( rootNode->mRoot )
        rootNode = rootNode->mRoot;
    partNode* foundNode;
    rootNode->calcNodeIdOrFindNode( 0, 0, id, &foundNode );
    return foundNode;
}


int partNode::calcNodeIdOrFindNode( int oldId, const partNode* findNode, int findId, partNode** foundNode )
{
    // We use the same algorithm to determine the id of a node and
    //                           to find the node when id is known.
    int myId = oldId+1;
    // check for node ?
    if( findNode && this == findNode )
        return myId;
    // check for id ?
    if(  foundNode && myId == findId ) {
        *foundNode = this;
        return myId;
    }
    if( mChild )
        return mChild->calcNodeIdOrFindNode( myId, findNode, findId, foundNode );
    if( mNext )
        return mNext->calcNodeIdOrFindNode(  myId, findNode, findId, foundNode );

    if(  foundNode )
        *foundNode = 0;
    return -1;
}


partNode* partNode::findType( int type, int subType, bool deep, bool wide )
{
    if(    (mType != DwMime::kTypeUnknown)
           && (    (type == DwMime::kTypeUnknown)
                   || (type == mType) )
           && (    (subType == DwMime::kSubtypeUnknown)
                   || (subType == mSubType) ) )
        return this;
    else if( mChild && deep )
        return mChild->findType( type, subType, deep, wide );
    else if( mNext && wide )
        return mNext->findType(  type, subType, deep, wide );
    else
        return 0;
}

partNode* partNode::findTypeNot( int type, int subType, bool deep, bool wide )
{
    if(    (mType != DwMime::kTypeUnknown)
           && (    (type == DwMime::kTypeUnknown)
                   || (type != mType) )
           && (    (subType == DwMime::kSubtypeUnknown)
                   || (subType != mSubType) ) )
        return this;
    else if( mChild && deep )
        return mChild->findTypeNot( type, subType, deep, wide );
    else if( mNext && wide )
        return mNext->findTypeNot(  type, subType, deep, wide );
    else
        return 0;
}

void partNode::fillMimePartTree( KMMimePartTreeItem* parentItem,
                                 KMMimePartTree*     mimePartTree,
                                 QString labelDescr,
                                 QString labelCntType,
                                 QString labelEncoding,
                                 QString labelSize )
{
  if( parentItem || mimePartTree ) {

    if( mNext )
        mNext->fillMimePartTree( parentItem, mimePartTree );

    QString cntDesc, cntType, cntEnc, cntSize;

    if( labelDescr.isEmpty() ) {
        DwHeaders* headers = 0;
        if( mDwPart && mDwPart->hasHeaders() )
          headers = &mDwPart->Headers();
        if( headers && headers->HasSubject() )
            cntDesc = headers->Subject().AsString().c_str();
        if( headers && headers->HasContentType()) {
            cntType = headers->ContentType().TypeStr().c_str();
            cntType += '/';
            cntType += headers->ContentType().SubtypeStr().c_str();
            if( 0 < headers->ContentType().Name().length() ) {
                if( !cntDesc.isEmpty() )
                    cntDesc += ", ";
                cntDesc = headers->ContentType().Name().c_str();
            }
        }
        else
            cntType = "text/plain";
        if( headers && headers->HasContentDescription()) {
            if( !cntDesc.isEmpty() )
                cntDesc += ", ";
            cntDesc = headers->ContentDescription().AsString().c_str();
        }
        if( cntDesc.isEmpty() ) {
            bool bOk = false;
            if( headers && headers->HasContentDisposition() ) {
                QCString dispo = headers->ContentDisposition().AsString().c_str();
                int i = dispo.find("filename=", 0, false);
                if( -1 < i ) {  //  123456789
                    QCString s = dispo.mid( i + 9 ).stripWhiteSpace();
                    if( '\"' == s[0])
                        s.remove( 0, 1 );
                    if( '\"' == s[s.length()-1])
                        s.truncate( s.length()-1 );
                    if( !s.isEmpty() ) {
                        cntDesc  = "file: ";
                        cntDesc += s;
                        bOk = true;
                    }
                }
            }
            if( !bOk ) {
                if( mRoot && mRoot->mRoot )
                    cntDesc = i18n("internal part");
                else
                    cntDesc = i18n("body part");
            }
        }
        if(    mDwPart
            && mDwPart->hasHeaders()
            && mDwPart->Headers().HasContentTransferEncoding() )
            cntEnc = mDwPart->Headers().ContentTransferEncoding().AsString().c_str();
        else
            cntEnc = "7bit";
        if( mDwPart )
            cntSize = QString::number( mDwPart->Body().AsString().length() );
    } else {
        cntDesc = labelDescr;
        cntType = labelCntType;
        cntEnc  = labelEncoding;
        cntSize = labelSize;
    }

    cntType = KMComposeWin::prettyMimeType( cntType );

    cntDesc += "  ";
    cntType += "  ";
    cntEnc  += "  ";
kdDebug(5006) << "      Inserting one item into MimePartTree" << endl;
kdDebug(5006) << "                Content-Type: " << cntType << endl;
    if( parentItem )
      mMimePartTreeItem = new KMMimePartTreeItem( *parentItem,
                                                  this,
                                                  cntDesc,
                                                  cntType,
                                                  cntEnc,
                                                  cntSize );
    else if( mimePartTree )
      mMimePartTreeItem = new KMMimePartTreeItem( *mimePartTree,
                                                  this,
                                                  cntDesc,
                                                  cntType,
                                                  cntEnc,
                                                  cntSize );
    mMimePartTreeItem->setOpen( true );
    if( mChild )
        mChild->fillMimePartTree( mMimePartTreeItem, 0 );

  }
}

void partNode::adjustDefaultType( partNode* node )
{
    // Only bodies of  'Multipart/Digest'  objects have
    // default type 'Message/RfC822'.  All other bodies
    // have default type 'Text/Plain'  (khz, 5.12.2001)
    if( node && DwMime::kTypeUnknown == node->type() ) {
        if(    node->mRoot
               && DwMime::kTypeMultipart == node->mRoot->type()
               && DwMime::kSubtypeDigest == node->mRoot->subType() ) {
            node->setType(    DwMime::kTypeMessage   );
            node->setSubType( DwMime::kSubtypeRfc822 );
        }
        else
            {
                node->setType(    DwMime::kTypeText     );
                node->setSubType( DwMime::kSubtypePlain );
            }
    }
}

bool partNode::isAttachment() const
{
  if (!dwPart())
	  return false;
  DwHeaders& headers = dwPart()->Headers();
  if( headers.HasContentDisposition() )
    return (headers.ContentDisposition().DispositionType() == DwMime::kDispTypeAttachment);
  else
    return false;
}

const QString& partNode::trueFromAddress() const
{
  const partNode* node = this;
  while( node->mFromAddress.isEmpty() && node->mRoot )
    node = node->mRoot;
  return node->mFromAddress;
}
