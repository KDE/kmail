// kmfawidgets.h - KMFilterAction parameter widgets
// Copyright: (c) 2001 Marc Mutz <Marc@Mutz.com>
// License: GPL

#ifndef _kmfawidgets_h_
#define _kmfawidgets_h_

#include <klineedit.h>
#include <qstring.h>

/** The param widget for @see KMFilterActionWithAddress..
    @author Marc Mutz <mutz@kde.org>
*/

class QPushButton;
class KURLRequester;

class KMFilterActionWithAddressWidget : public QWidget
{
  Q_OBJECT
public:
  KMFilterActionWithAddressWidget( QWidget* parent=0, const char* name=0 );

  void clear() { mLineEdit->clear(); }
  QString text() const { return mLineEdit->text(); }
  void setText( const QString & aString ) { mLineEdit->setText( aString ); }

protected slots:
  void slotAddrBook();

private:
  QPushButton* mBtn;
  QLineEdit*   mLineEdit;
};

class KMSoundTestWidget : public QWidget
{
  Q_OBJECT
public:
  KMSoundTestWidget( QWidget * parent, const char * name=0 );
  ~KMSoundTestWidget();
  QString url() const;
  void setUrl( const QString & url );
  void clear();
signals:
  void testPressed();
protected slots:
  void playSound();
  void openSoundDialog( KURLRequester * );
  void slotUrlChanged( const QString & );

private:
  KURLRequester *m_urlRequester;
  QPushButton *m_playButton;
};

#endif /*_kmfawidget_h_*/
