#include "kmtextbrowser.h"

#include <qaccel.h>

#include <kapplication.h>
#include <kwin.h>

KMTextBrowser::KMTextBrowser( QWidget *parent, const char *name )
    : KTextBrowser( parent, name )
{
    setWFlags( WDestructiveClose );
    QAccel *accel = new QAccel( this, "browser close-accel" );
    accel->connectItem( accel->insertItem( Qt::Key_Escape ), this , SLOT( close() ));
    setTextFormat( Qt::PlainText );
    setWordWrap( KTextBrowser::NoWrap );

    KWin::setIcons(winId(), kapp->icon(), kapp->miniIcon());
}
