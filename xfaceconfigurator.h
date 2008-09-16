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
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#ifndef __KMAIL_XFACECONFIGURATOR_H__
#define __KMAIL_XFACECONFIGURATOR_H__

#include <QWidget>

class KComboBox;
class KTextEdit;
class KUrl;

class QCheckBox;
class QString;
class QLabel;

namespace KMail {

  class XFaceConfigurator : public QWidget {
    Q_OBJECT
  public:
    XFaceConfigurator( QWidget * parent=0 );
    virtual ~XFaceConfigurator();

    bool isXFaceEnabled() const;
    void setXFaceEnabled( bool enable );


    QString xface() const;
    void setXFace( const QString & text );

  protected:
    QCheckBox     * mEnableCheck;
    KTextEdit     * mTextEdit;
    QLabel        * mXFaceLabel;
    KComboBox     * mSourceCombo;


  private:
    void setXfaceFromFile( const KUrl &url );

  private slots:
    void slotSelectFile();
    void slotSelectFromAddressbook();
    void slotUpdateXFace();
  };
} // namespace KMail

#endif // __KMAIL_XFACECONFIGURATOR_H__


