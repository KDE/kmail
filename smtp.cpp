/****************************************************************************
**
** This file is a modified version of part of an example program for Qt.
** This file may be used, distributed and modified without limitation.
**
** Don Sanders <sanders@kde.org>
**
*****************************************************************************/

#include "smtp.h"

#include <qtextstream.h>
#include <qsocket.h>
#include <qtimer.h>
#include <kapp.h>
#include <kmessagebox.h>
#include <klocale.h>

Smtp::Smtp( const QString &from, const QStringList &to,
	    const QString &aMessage,
	    const QString &server,
	    unsigned short int port )
{
    skipReadResponse = false;
    mSocket = new QSocket( this );
    connect ( mSocket, SIGNAL( readyRead() ),
	      this, SLOT( readyRead() ) );
    connect ( mSocket, SIGNAL( connected() ),
	      this, SLOT( connected() ) );
    connect ( mSocket, SIGNAL( error(int) ),
	      this, SLOT( socketError(int) ) );

    message = aMessage;
    
    this->from = from;
    rcpt = to;
    state = smtpInit;
    command = "";

    emit status( i18n( "Connecting to %1" ).arg( server ) );

    mSocket->connectToHost( server, port );
    t = new QTextStream( mSocket );
    t->setEncoding(QTextStream::Latin1);
}


Smtp::~Smtp()
{
    if (t)
	delete t;
    if (mSocket)
	delete mSocket;
}


void Smtp::send( const QString &from, const QStringList &to,
	    const QString &aMessage )
{
    skipReadResponse = true;
    message = aMessage;
    this->from = from;
    rcpt = to;

    state = smtpMail;
    command = "";
    readyRead();
}


void Smtp::quit()
{
    skipReadResponse = true;
    state = smtpQuit;
    command = "";
    readyRead();	
}


void Smtp::connected()
{
    emit status( i18n( "Connected to %1" ).arg( mSocket->peerName() ) );
}

void Smtp::socketError(int errorCode)
{
    command = "CONNECT";
    switch ( errorCode ) {
        case QSocket::ErrConnectionRefused:
	    responseLine = i18n( "Connection refused." );
	    break;
        case QSocket::ErrHostNotFound:
	    responseLine = i18n( "Host Not Found." );
	    break;
        case QSocket::ErrSocketRead:
	    responseLine = i18n( "Error reading socket." );
	    break;
        default:
	    responseLine = i18n( "Internal error, unrecognized error." );
    }
    QTimer::singleShot( 0, this, SLOT(emitError()) );
}

void Smtp::emitError() {
    error( command, responseLine );
}

void Smtp::readyRead()
{
    if (!skipReadResponse) {
	// SMTP is line-oriented
	if ( !mSocket->canReadLine() )
	    return;

	do {
	    responseLine = mSocket->readLine();
	    response += responseLine;
	} while( mSocket->canReadLine() && responseLine[3] != ' ' );
    }
    skipReadResponse = false;
	
    if ( state == smtpInit && responseLine[0] == '2' ) {
	// banner was okay, let's go on
	command = "HELO there";
	*t << "HELO there\r\n";
	state = smtpMail;
    } else if ( state == smtpMail && responseLine[0] == '2' ) {
	// HELO response was okay (well, it has to be)
	command = "MAIL";
	*t << "MAIL FROM: <" << from << ">\r\n";
	state = smtpRcpt;
    } else if ( state == smtpRcpt && responseLine[0] == '2' && (rcpt.begin() != rcpt.end())) {
	command = "RCPT";
	*t << "RCPT TO: <" << *(rcpt.begin()) << ">\r\n";
	rcpt.remove( rcpt.begin() );
	if (rcpt.begin() == rcpt.end())
	    state = smtpData;
    } else if ( state == smtpData && responseLine[0] == '2' ) {
	command = "DATA";
	*t << "DATA\r\n";
	state = smtpBody;
    } else if ( state == smtpBody && responseLine[0] == '3' ) {
	command = "DATA";
	QString seperator = "";
	if (message[message.length() - 1] != '\n')
	    seperator = "\r\n";
	*t << message << seperator << ".\r\n";
	state = smtpSuccess;
    } else if ( state == smtpSuccess && responseLine[0] == '2' ) {
	QTimer::singleShot( 0, this, SIGNAL(success()) );
    } else if ( state == smtpQuit && responseLine[0] == '2' ) {
	command = "QUIT";
	*t << "QUIT\r\n";
	// here, we just close.
	state = smtpClose;
	emit status( tr( "Message sent" ) );
    } else if ( state == smtpClose ) {
	// we ignore it
    } else { // error occurred
	QTimer::singleShot( 0, this, SLOT(emitError()) );
	state = smtpClose;
    }

    response = "";

    if ( state == smtpClose ) {
	delete t;
	t = 0;
	delete mSocket;
	mSocket = 0;
	QTimer::singleShot( 0, this, SLOT(deleteMe()) );
    }
}


void Smtp::deleteMe()
{
    delete this;
}

#include "smtp.moc"
