/*
    This file is part of KMail.

    Copyright (c) 2004 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
    
    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/
#ifndef RECIPIENTSEDITOR_H
#define RECIPIENTSEDITOR_H

#include <qwidget.h>
#include <qscrollview.h>
#include <qlineedit.h>

#include "kmcomposewin.h"

class RecipientsPicker;

class KWindowPositioner;

class QComboBox;
class QLabel;

class Recipient
{
  public:
    typedef QValueList<Recipient> List;

    enum Type { To, Cc, Bcc, ReplyTo };

    Recipient( const QString &email = QString::null, Type type = To );

    void setType( Type );
    Type type() const;

    void setEmail( const QString & );
    QString email() const;

    bool isEmpty() const;

    static int typeToId( Type );
    static Type idToType( int );
    
    QString typeLabel() const;    static QString typeLabel( Type );
    static QStringList allTypeLabels();
    
  private:
    QString mEmail;
    Type mType;
};

/* Helper Class */

class RecipientLineEdit : public KMLineEdit
{
  Q_OBJECT
  public:
    RecipientLineEdit(  QWidget * parent ) :
      KMLineEdit( true, parent ) { }

  signals:
    void deleteMe();

  protected:
    void keyPressEvent(  QKeyEvent *ev );
};

class RecipientLine : public QWidget
{
    Q_OBJECT
  public:
    RecipientLine( QWidget *parent );

    void setRecipient( const Recipient & );
    Recipient recipient() const;

    void setRecipient( const QString & );

    void activate();
    bool isActive();

    bool isEmpty();

  signals:
    void returnPressed( RecipientLine * );
    void downPressed( RecipientLine * );
    void upPressed( RecipientLine * );
    void deleteLine(  RecipientLine * );
    void emptyChanged();


  protected:
    void keyPressEvent( QKeyEvent * );

  protected slots:
    void slotReturnPressed();
    void checkEmptyState( const QString & );
    void slotFocusUp();
    void slotFocusDown();
    void slotPropagateDeletion();

  private:
    QComboBox *mCombo;
    RecipientLineEdit *mEdit;
    bool mIsEmpty;
};

class RecipientsView : public QScrollView
{
    Q_OBJECT
  public:
    RecipientsView( QWidget *parent );

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

    RecipientLine *activeLine();

    RecipientLine *emptyLine();

    Recipient::List recipients() const;

    void activateLine( RecipientLine * );

  public slots:
    RecipientLine *addLine();

    void setFocus();
    void setFocusTop();
    void setFocusBottom();

  signals:
    void totalChanged( int recipients, int lines );
    void focusUp();
    void focusDown();

  protected:
    void viewportResizeEvent( QResizeEvent * );
    void resizeView();

  protected slots:
    void slotReturnPressed( RecipientLine * );
    void slotDownPressed( RecipientLine * );
    void slotUpPressed( RecipientLine * );
    void slotDecideLineDeletion(  RecipientLine * );
    void slotDeleteDueLine();
    void calculateTotal();

  private:
    QPtrList<RecipientLine> mLines;
    RecipientLine* mCurDelLine;
    int mLineHeight;
};

class SideWidget : public QWidget
{
    Q_OBJECT
  public:
    SideWidget( RecipientsView *view, QWidget *parent );

  public slots:
    void setTotal( int recipients, int lines );

  signals:
    void pickedRecipient( const QString & );

  protected:
    void initRecipientPicker();
    
  protected slots:
    void pickRecipient();

  private:
    RecipientsView *mView;
    QLabel *mTotalLabel;
    RecipientsPicker *mRecipientPicker;
    KWindowPositioner *mPickerPositioner;
};

class RecipientsEditor : public QWidget
{
    Q_OBJECT
  public:
    RecipientsEditor( QWidget *parent );

    void clear();

    Recipient::List recipients() const;

    void setRecipientString( const QString &, Recipient::Type );
    QString recipientString( Recipient::Type );

  public slots:
    void setFocus();
    void setFocusTop();
    void setFocusBottom();

  signals:
    void focusUp();
    void focusDown();

  protected slots:
    void slotPickedRecipient( const QString & );

  private:
    RecipientsView *mRecipientsView;
};

#endif
