#include <kdebug.h>
#include <kmmessage.h>
#include <qregexp.h>

typedef QString (*MagicDetectorFunc) (const KMMessage *, QString &, QString &);

/* Sender: (owner-([^@]+)|([^@+]-owner)@ */
static QString check_sender(const KMMessage  *message,
                     QString &header_name,
                     QString &header_value )
{
    QString header = message->headerField( "Sender" );

    if ( header.isEmpty() )
        return QString::null;

    if ( header.left( 6 ) == "owner-" )
    {
        header_name = "Sender";
        header_value = header;
        header = header.mid( 6, header.find( "@" ) - 6 );

    } else {
        int index = header.find( "-owner@ " );
        if ( index == -1 )
            return QString::null;

        header = header.left( index );
        header_name = "Sender";
        header_value = header;
    }

    return header;
}

/* X-BeenThere: ([^@]+) */
static QString check_x_beenthere(const KMMessage  *message,
                                 QString &header_name,
                                 QString &header_value )
{
    QString header = message->headerField( "X-BeenThere" );
    if ( header.isNull() || header.find( "@" ) == -1 )
        return QString::null;

    header_name = "X-BeenThere";
    header_value = header;
    header = header.left( header.find( "@" ) );
    return header;
}

/* Delivered-To:: <([^@]+) */
static QString check_delivered_to(const KMMessage  *message,
                                  QString &header_name,
                                  QString &header_value )
{
    QString header = message->headerField( "Delivered-To" );
    if ( header.isNull() || header.left(13 ) != "mailing list"
         || header.find( "@" ) == -1 )
        return QString::null;

    header_name = "Delivered-To";
    header_value = header;

    return header.mid( 13, header.find( "@" ) - 13 );
}

/* X-Mailing-List: <?([^@]+) */
static QString check_x_mailing_list(const KMMessage  *message,
                                    QString &header_name,
                                    QString &header_value )
{
    QString header = message->headerField( "X-Mailing-List");
    if ( header.isEmpty() )
        return QString::null;

    if ( header.find( "@" ) < 1 )
        return QString::null;

    header_name = "X-Mailing-List";
    header_value = header;
    if ( header[0] == '<' )
        header = header.mid(1,  header.find( "@" ) - 1);
    else
        header = header.left( header.find( "@" ) );
    return header;
}

/* List-Id: [^<]* <([^.]+) */
static QString check_list_id(const KMMessage  *message,
			     QString &header_name,
			     QString &header_value )
{
    int lAnglePos, firstDotPos;
    QString header = message->headerField( "List-Id" );
    if ( header.isEmpty() )
        return QString::null;

    lAnglePos = header.find( "<" );
    if ( lAnglePos < 0 )
        return QString::null;

    firstDotPos = header.find( ".", lAnglePos );
    if ( firstDotPos < 0 )
        return QString::null;

    header_name = "List-Id";
    header_value = header;
    header = header.mid( lAnglePos + 1, firstDotPos - lAnglePos - 1 );
    return header;
}

/* Mailing-List: list ([^@]+) */
static QString check_mailing_list(const KMMessage  *message,
                                    QString &header_name,
                                    QString &header_value )
{
    QString header = message->headerField( "Mailing-List");
    if ( header.isEmpty() )
        return QString::null;

    if (header.left( 5 ) != "list " || header.find( "@" ) < 5 )
        return QString::null;

    header_name = "Mailing-List";
    header_value = header;
    header = header.mid(5,  header.find( "@" ) - 5);
    return header;
}


/* X-Loop: ([^@]+) */
static QString check_x_loop(const KMMessage  *message,
                                    QString &header_name,
                                    QString &header_value ){
    QString header = message->headerField( "X-Loop");
    if ( header.isEmpty() )
        return QString::null;

    if (header.find( "@" ) < 2 )
        return QString::null;

    header_name = "X-Loop";
    header_value = header;
    header = header.left(header.find( "@" ));
    return header;
}

MagicDetectorFunc magic_detector[] =
{
    check_list_id,
    check_sender,
    check_x_mailing_list,
    check_mailing_list,
    check_delivered_to,
    check_x_beenthere,
    check_x_loop
};

static const int num_detectors = sizeof (magic_detector) / sizeof (magic_detector[0]);

QString detect_list(const KMMessage  *message,
                    QString &header_name,
                    QString &header_value )
{
    header_name = QString::null;
    header_value = QString::null;

    if ( !message )
        return QString::null;

    QString list;

    for (int i = 0; i < num_detectors; i++) {
        list = magic_detector[i] (message, header_name, header_value);
        if ( !list.isNull() )
            return list;
    }

    return QString::null;
}
