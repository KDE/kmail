#include "recipientseditor.h"

#include <kaboutdata.h>
#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kcmdlineargs.h>

#include "qpushbutton.h"

int main( int argc, char **argv )
{
  KAboutData aboutData( "testrecipienteditortestaddresseeseletor",
   "Test Recipient Editor", "0.1" );
  KCmdLineArgs::init( argc, argv, &aboutData );

  KApplication app;

  QObject::connect( &app, SIGNAL( lastWindowClosed() ), &app, SLOT( quit() ) );

  QWidget *wid = new RecipientsEditor( 0 );
  
  wid->show();

  return app.exec();
}
