/*  -*- c++ -*-
    xfaceconfigurator.cpp

    KMail, the KDE mail client.
    Copyright (c) 2004 Jakob Schröter <js@camaya.net>
    Copyright (c) 2002 the KMail authors.
    See file AUTHORS for details

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#ifndef __KMAIL_XFACECONFIGURATOR_H__
#define __KMAIL_XFACECONFIGURATOR_H__

#include <qwidget.h>
#include <qtextedit.h>

class KURL;

class QCheckBox;
class QString;
class QLabel;
class QComboBox;

namespace KMail {

  class XFaceConfigurator : public QWidget {
    Q_OBJECT
  public:
    XFaceConfigurator( QWidget * parent=0, const char * name=0 );
    virtual ~XFaceConfigurator();

    bool isXFaceEnabled() const;
    void setXFaceEnabled( bool enable );


    QString xface() const;
    void setXFace( const QString & text );

  protected:
    QCheckBox     * mEnableCheck;
    QTextEdit     * mTextEdit;
    QLabel        * mXFaceLabel;
    QComboBox     * mSourceCombo;


  private:
    void setXfaceFromFile( const KURL &url );

  private slots:
    void slotSelectFile();
    void slotSelectFromAddressbook();
    void slotUpdateXFace();
  };
} // namespace KMail

#endif // __KMAIL_XFACECONFIGURATOR_H__


