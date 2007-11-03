#include "filterimporterexporter.h"

#include "kmfilter.h"
#include "kmfilteraction.h"
#include "util.h"

#include <kconfig.h>
#include <kdebug.h>
#include <kfiledialog.h>

#include <qregexp.h>


using namespace KMail;

/* static */
QValueList<KMFilter*> FilterImporterExporter::readFiltersFromConfig( KConfig* config, bool bPopFilter )
{
    int numFilters = 0;
    if (bPopFilter)
      numFilters = config->readNumEntry("popfilters",0);
    else
      numFilters = config->readNumEntry("filters",0);
    
    QValueList<KMFilter*> filters;
    for ( int i=0 ; i < numFilters ; ++i ) {
      QString grpName;
      grpName.sprintf("%s #%d", (bPopFilter ? "PopFilter" : "Filter") , i);
      KConfigGroupSaver saver(config, grpName);
      KMFilter * filter = new KMFilter(config, bPopFilter);
      filter->purify();
      if ( filter->isEmpty() ) {
    #ifndef NDEBUG
        kdDebug(5006) << "KMFilter::readConfig: filter\n" << filter->asString()
          << "is empty!" << endl;
    #endif
        delete filter;
      } else
        filters.append(filter);
    }
    return filters;
}

/* static */ 
void FilterImporterExporter::writeFiltersToConfig( const QValueList<KMFilter*>& filters, KConfig* config, bool bPopFilter )
{
    // first, delete all groups:
    QStringList filterGroups =
      config->groupList().grep( QRegExp( bPopFilter ? "PopFilter #\\d+" : "Filter #\\d+" ) );
    for ( QStringList::Iterator it = filterGroups.begin() ;
          it != filterGroups.end() ; ++it )
      config->deleteGroup( *it );
    
    int i = 0;
    for ( QValueListConstIterator<KMFilter*> it = filters.constBegin() ;
          it != filters.constEnd() ; ++it ) {
      if ( !(*it)->isEmpty() ) {
        QString grpName;
        if ( bPopFilter )
          grpName.sprintf("PopFilter #%d", i);
        else
          grpName.sprintf("Filter #%d", i);
        KConfigGroupSaver saver(config, grpName);
        (*it)->writeConfig(config);
        ++i;
      }
    }
    if (bPopFilter)
      config->writeEntry("popfilters", i);
    else
      config->writeEntry("filters", i);
}


FilterImporterExporter::FilterImporterExporter( QWidget* parent, bool popFilter )
:mParent( parent), mPopFilter( popFilter )
{
}

FilterImporterExporter::~FilterImporterExporter()
{
}

QValueList<KMFilter*> FilterImporterExporter::importFilters()
{
    QString fileName = KFileDialog::getOpenFileName( QDir::homeDirPath(), QString::null, mParent, i18n("Import Filters") );
    if ( fileName.isEmpty() ) return QValueList<KMFilter*>();
    
    KConfig config( fileName );
    QValueList<KMFilter*> imported = readFiltersFromConfig( &config, mPopFilter );
    // FIXME ask user to confirm which ones to import
    return imported;
}

void FilterImporterExporter::exportFilters(const QValueList<KMFilter*> & filters )
{
    KURL saveUrl = KFileDialog::getSaveURL( QDir::homeDirPath(), QString::null, mParent, i18n("Export Filters") );
    
    if ( saveUrl.isEmpty() || !Util::checkOverwrite( saveUrl, 0 ) )
      return;
    
    KConfig config( saveUrl.path() );
    // FIXME ask user to confirm which ones to export
    writeFiltersToConfig( filters, &config, mPopFilter );
}

