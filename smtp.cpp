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
    socket = new QSocket( this );
    connect ( socket, SIGNAL( readyRead() ),
	      this, SLOT( readyRead() ) );
    connect ( socket, SIGNAL( connected() ),
	      this, SLOT( connected() ) );
    connect ( socket, SIGNAL( error(int) ),
	      this, SLOT( socketError(int) ) );

    message = aMessage;
    
    this->from = from;
    rcpt = to;

    state = Init;
    command = "";

    emit status( i18n( "Connecting to %1" ).arg( server ) );

    socket->connectToHost( server, port );
    t = new QTextStream( socket );
}


Smtp::~Smtp()
{
    if (t)
	delete t;
    if (socket)
	delete socket;
}


void Smtp::send( const QString &from, const QStringList &to,
	    const QString &aMessage )
{
    skipReadResponse = true;
    message = aMessage;
    this->from = from;
    rcpt = to;

    state = Mail;
    command = "";
    readyRead();
}


void Smtp::quit()
{
    skipReadResponse = true;
    state = Quit;
    command = "";
    readyRead();	
}


void Smtp::connected()
{
    emit status( i18n( "Connected to %1" ).arg( socket->peerName() ) );
}

void Smtp::socketError(int errorCode)
{
    command = "CONNECT";
    responseLine;
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
	if ( !socket->canReadLine() )
	    return;

	do {
	    responseLine = socket->readLine();
	    response += responseLine;
	} while( socket->canReadLine() && responseLine[3] != ' ' );
	responseLine.truncate( 3 );
    }
    skipReadResponse = false;
	
    if ( state == Init && responseLine[0] == '2' ) {
	// banner was okay, let's go on
	command = "HELO there";
	*t << "HELO there\r\n";
	state = Mail;
    } else if ( state == Mail && responseLine[0] == '2' ) {
	// HELO response was okay (well, it has to be)
	command = "MAIL";
	*t << "MAIL FROM: <" << from << ">\r\n";
	state = Rcpt;
    } else if ( state == Rcpt && responseLine[0] == '2' && (rcpt.begin() != rcpt.end())) {
	command = "RCPT";
	*t << "RCPT TO: <" << *(rcpt.begin()) << ">\r\n";
	rcpt.remove( rcpt.begin() );
	if (rcpt.begin() == rcpt.end())
	    state = Data;
    } else if ( state == Data && responseLine[0] == '2' ) {
	command = "DATA";
	*t << "DATA\r\n";
	state = Body;
    } else if ( state == Body && responseLine[0] == '3' ) {
	command = "DATA";
	QString seperator = "";
	if (message[message.length() - 1] != '\n')
	    seperator = "\n";
	*t << message << seperator << ".\r\n";
	state = Success;
    } else if ( state == Success && responseLine[0] == '2' ) {
	QTimer::singleShot( 0, this, SIGNAL(success()) );
    } else if ( state == Quit && responseLine[0] == '2' ) {
	command = "QUIT";
	*t << "QUIT\r\n";
	// here, we just close.
	state = Close;
	emit status( tr( "Message sent" ) );
    } else if ( state == Close ) {
	// we ignore it
    } else {
	state = Close;
    }

    response = "";

    if ( state == Close ) {
	delete t;
	t = 0;
	delete socket;
	socket = 0;
	QTimer::singleShot( 0, this, SLOT(deleteMe()) );
    }
}


void Smtp::deleteMe()
{
    delete this;
}

