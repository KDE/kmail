/****************************************************************************
**
** This file is a modified version of part of an example program for Qt.
** This file may be used, distributed and modified without limitation.
**
** Don Sanders <sanders@kde.org>
**
*****************************************************************************/

#ifndef SMTP_H
#define SMTP_H

#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>

class QSocket;
class QTextStream;

class Smtp : public QObject
{
    Q_OBJECT

public:
    Smtp( const QString &, const QStringList &, const QString &, const QString &,
          unsigned short int );
    ~Smtp();
    void send( const QString &, const QStringList &, const QString & );
    void quit();


signals:
    void success();
    void status( const QString & );
    void error( const QString &command, const QString &response );

private slots:
    void readyRead();
    void connected();
    void deleteMe();
    void socketError(int err);
    void emitError();

private:
    enum State {
	Init,
	Mail,
	Rcpt,
	Data,
	Body,
	Success,
	Quit,
	Close
    };

    QString message;
    QString from;
    QStringList rcpt;
    QSocket *socket;
    QTextStream * t;
    int state;
    QString response, responseLine;
    bool skipReadResponse;
    QString command;
};

#endif
