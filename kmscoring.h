/*
 *   kmscoring.h
 *
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2000 Espen Sand, espen@kde.org
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software Foundation,
 *   Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, US
 */

#ifndef KMSCORING_H
#define KMSCORING_H

#include <qmap.h>
#include <kscoring.h>

class QWidget;
class KMMainWin;
class KDialogBase;

class KMScorableArticle : public ScorableArticle
{
public:
  KMScorableArticle(const QCString&);
  virtual ~KMScorableArticle();

  virtual void addScore(short s);
  virtual QString from() const;
  virtual QString subject() const;
  virtual QString getHeaderByType(const QString&) const;

  int score() const { return mScore; }

private:
  QCString mMsgStr;
  int mScore;
  mutable QMap<QString, QString> mParsedHeaders;
};


class KMScorableGroup : public ScorableGroup
{
public:
  KMScorableGroup();
  virtual ~KMScorableGroup();    
};


class KMScoringManager : public KScoringManager
{
  Q_OBJECT

public:
  KMScoringManager();
  virtual ~KMScoringManager();
  virtual QStringList getGroups() const;

  void setMainWin(QObject *parent);
  
  void configure();

  static KMScoringManager* globalScoringManager();

protected:
  KDialogBase  *mConfDialog;
  KMMainWin *mMainWin;

  static KMScoringManager *mScoringManager;
    
protected slots:
  void slotDialogDone();

};

#endif
