/*  -*- c++ -*-
    signatureconfigurator.cpp

    KMail, the KDE mail client.
    Copyright (c) 2002 the KMail authors.
    See file AUTHORS for details

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/

#ifndef __KMAIL_SIGNATURECONFIGURATOR_H__
#define __KMAIL_SIGNATURECONFIGURATOR_H__

#include <qwidget.h>

#include <libkpimidentities/identity.h> // for Signature::Type
using KPIM::Signature;

class QComboBox;
class QCheckBox;
class KURLRequester;
class KLineEdit;
class QString;
class QPushButton;
class QTextEdit;

namespace KMail {

  class SignatureConfigurator : public QWidget {
    Q_OBJECT
  public:
    SignatureConfigurator( QWidget * parent=0, const char * name=0 );
    virtual ~SignatureConfigurator();

    bool isSignatureEnabled() const;
    void setSignatureEnabled( bool enable );

    Signature::Type signatureType() const;
    void setSignatureType( Signature::Type type );

    QString inlineText() const;
    void setInlineText( const QString & text );

    QString fileURL() const;
    void setFileURL( const QString & url );

    QString commandURL() const;
    void setCommandURL( const QString & url );

    /**
       Conveniece method.
       @return a @see Signature object representing the state of the widgets.
     **/
    Signature signature() const;
    /**
       Convenience method. Sets the widgets according to @p sig
    **/
    void setSignature( const Signature & sig );

  protected slots:
    void slotEnableEditButton( const QString & );
    void slotEdit();

  protected:
    QCheckBox     * mEnableCheck;
    QComboBox     * mSourceCombo;
    KURLRequester * mFileRequester;
    QPushButton   * mEditButton;
    KLineEdit     * mCommandEdit;
    QTextEdit     * mTextEdit;
  };

} // namespace KMail

#endif // __KMAIL_SIGNATURECONFIGURATOR_H__


