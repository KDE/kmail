// kmfawidgets.h - KMFilterAction parameter widgets
// Copyright: (c) 2001 Marc Mutz <Marc@Mutz.com>
// License: GPL

#ifndef _kmfawidgets_h_
#define _kmfawidgets_h_

#include <klineedit.h>
#include <qstring.h>

/** The param widget for @ref KMFilterActionWithAddress..
    @author Marc Mutz <mutz@kde.org>
*/

class QPushButton;

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


#endif /*_kmfawidget_h_*/
