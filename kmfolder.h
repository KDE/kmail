/* Local Mail folder
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
 */
#ifndef kmfolder_h
#define kmfolder_h

#include "kmfoldernode.h"
#include "kmmsginfo.h"
#include <stdio.h>
#include <qarray.h>

typedef QArray<KMMessage> KMMessageList;

class KMMessage;
class BasicMessage;
class KMFolderDir;

#define KMFolderInherited KMFolderNode

class KMFolder: public KMFolderNode
{
  Q_OBJECT

public:
  /** Usually a parent is given. But in some cases there is no
    fitting parent object available. Then the name of the folder
    is used as the absolute path to the folder file. */
  KMFolder(KMFolderDir* parent=NULL, const char* name=NULL);
  virtual ~KMFolder();

  /** Returns full path to folder file */
  const QString& location(void) const;

  /** Read message at given index. Indexing starts at one to stay
    compatible with imap-lib */
  virtual KMMessage* getMsg(int index);

  /** Detach message from this folder. Usable to call addMsg()
    with the message for another folder. */
  virtual void detachMsg(int index);

  /** Add the given message to the folder. Usually the message
    is added at the end of the folder. Returns zero on success and
    an errno error code on failure. The index of the new message
    is optionally returned. 
    Please note that the message is added as is to the folder and the folder
    takes ownership of the message (deleting it in the destructor).*/
  virtual int addMsg(KMMessage* msg, int* index = NULL);

  /** total number of messages in this folder (may include already deleted
   messages) */
  virtual long numMsgs(void) const { return mMsgs; }

  /** number of unread messages */
  virtual int numUnreadMsgs(void) const { return mUnreadMsgs; }

  /** number of active (not deleted) messages in folder */
  virtual int numActiveMsgs(void) const { return mActiveMsgs; }

  virtual int isValid(unsigned long);

  /** Open folder for access. Does not work if the parent is not set.
    Does nothing if the folder is already opened. To reopen a folder
    call close() first.
    Returns zero on success and an error code equal to the c-library
    fopen call otherwise. */
  virtual int open(void);

  /** Close folder. */
  virtual void close(void);

  /** Read the toc header only such that possible header information 
    (e.g. information about the associated accounts for KMAcctFolder)
    is set. The folder is *not* open afterwards. This method does nothing
    if the folder is already open and fails if the path was not set
    with setPath. */
  virtual int readHeader(void);

  /** Create a new folder with the name of this object and open it.
      Returns zero on success and an error code equal to the 
      c-library fopen call otherwise. */
  virtual int create(void);

  /** Removes the folder physically from disk and empties the contents
    of the folder in memory. Note that the folder is closed during this
    process, whether there are others using it or not. */
  virtual int remove(void);

  /** Returns TRUE if a table of contents file is automatically created. */
  bool autoCreateToc(void) const { return mAutoCreateToc; }

  /** Allow/disallow automatic creation of a table of contents file.
    Default is TRUE. */
  virtual void setAutoCreateToc(bool);


  //---| yet not implemented are: |--------------------------------------
  virtual int rename(const char* fileName);
  virtual long status(long/* = SA_MESSAGES | SA_RECENT | SA_UNSEEN*/);
  virtual void ping();
  virtual void expunge();

signals:
  /** Emitted when the status, name, or associated accounts of this
    folder changed. */
  void changed();

  /** Emitted when a message is removed from the folder. */
  void msgRemoved(int id);

  /** Emitted when a message is added from the folder. */
  void msgAdded(int id);

  /** Emitted when a field of the header of a specific message changed. */
  void msgHeaderChanged(int id);

protected:
  // read message from file
  virtual void readMsg(int msgNo);

  /* read table of contents file from messages file and fill the
     message-info list mMsgList. */
  virtual void readToc(void);

  /* This method can be inherited to read a custom toc header. */
  virtual void readTocHeader(void);

  /* Create table of contents file from messages file and fill the
     message-info list mMsgList. Returns 0 on success and an errno 
     value (see fopen) on failure. */
  virtual int createToc(void);

  /* This method can be inherited to create a custom toc header.
     Returns zero on success and an errno value (see fopen) on failure.*/
  virtual int createTocHeader(void);

  FILE* mStream;	// file with the messages
  FILE* mTocStream;	// table of contents file
  int mMsgs, mUnreadMsgs, mActiveMsgs;
  KMMsgInfoList mMsgInfo;
  int mOpenCount;
  bool mAutoCreateToc;  // is the automatic creation of a toc file allowed ?
};

#endif /*kmfolder_h*/
