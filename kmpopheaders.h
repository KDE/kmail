/***************************************************************************
                          kmpopheaders.h
                             -------------------
    begin                : Mon Oct 22 2001
    copyright            : (C) 2001 by Heiko Hund
                                       Thorsten Zachmann
    email                : heiko@ist.eigentlich.net
                           T.Zachmann@zagge.de
 ***************************************************************************/

#ifndef KMPopHeaders_H
#define KMPopHeaders_H

#include <qstring.h>
#include "kmmessage.h"


enum KMPopFilterAction {Down=0, Later=1, Delete=2, NoAction=3}; //Keep these corresponding to the column numbers in the dialog for easier coding 
								//or change mapToAction and mapToColumn in KMPopHeadersView

class KMPopHeaders {
public:

  KMPopHeaders();
  ~KMPopHeaders();
  /** constructor */
  KMPopHeaders(QString aId, QString aUid, KMPopFilterAction aAction);

  /** returns the id of the message */
  QString id() const;

  /** returns the uid of the message */
  QString uid() const;

  /** returns the header of the message */
  KMMessage * header() const;

  /** set the header of the message */
  void setHeader(KMMessage *aHeader);

  /** No descriptions */
  KMPopFilterAction action() const;

  /** No descriptions */
  void setAction(KMPopFilterAction aAction);
  /** No descriptions */
  bool ruleMatched();
  /** No descriptions */
  void setRuleMatched(bool b);
protected: // Protected attributes
  /** */
  KMPopFilterAction mAction;
  /**  */
  QString mId;
  /**  */
  QString mUid;
  /**  */
  bool mRuleMatched;
  /**  */
  KMMessage *mHeader;
};

#endif
