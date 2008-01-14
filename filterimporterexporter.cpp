#include "filterimporterexporter.h"

#include "kmfilter.h"
#include "kmfilteraction.h"
#include "util.h"

#include <kconfig.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kdialog.h>
#include <QListWidget>

#include <QRegExp>


using namespace KMail;

class FilterSelectionDialog : public KDialog
{
public:
    FilterSelectionDialog( QWidget * parent = 0 )
        :KDialog( parent )
    {
        setObjectName( "filterselection" );
        setModal( true );
        setCaption( i18n("Select Filters") );
        setButtons( Ok|Cancel );
        setDefaultButton( Ok );
        showButtonSeparator( true );
        filtersListWidget = new QListWidget( this );
        setMainWidget( filtersListWidget );
        filtersListWidget->setSortingEnabled( false );
        filtersListWidget->setSelectionMode( QAbstractItemView::NoSelection );
        //### filtersListWidget->addColumn( i18n("Filters"), 300 );
        //### filtersListWidget->setFullWidth( true );
        resize( 300, 350 );
    }

    virtual ~FilterSelectionDialog()
    {
    }

    void setFilters( const QList<KMFilter *> &filters )
    {
        originalFilters = filters;
        filtersListWidget->clear();
        foreach ( KMFilter *const filter, filters ) {
            QListWidgetItem *item = new QListWidgetItem( filter->name(), filtersListWidget );
            item->setFlags( Qt::ItemIsUserCheckable | Qt::ItemIsEnabled );
            item->setCheckState( Qt::Checked );
        }
    }
    
    QList<KMFilter *> selectedFilters() const
    {
        QList<KMFilter *> filters;
        for ( int i = 0; i < filtersListWidget->count(); i++ ) {
            QListWidgetItem *item = filtersListWidget->item( i );
            if ( item->checkState() == Qt::Checked )
                filters << originalFilters[i];
        }
        return filters;
    }
private:
    QListWidget *filtersListWidget;
    QList<KMFilter *> originalFilters;
};

/* static */
QList<KMFilter *> FilterImporterExporter::readFiltersFromConfig( KConfig *config, bool bPopFilter )
{
    KConfigGroup group = config->group( "General" );
    int numFilters = 0;
    if (bPopFilter)
      numFilters = group.readEntry( "popfilters", 0 );
    else
      numFilters = group.readEntry( "filters", 0 );
    
    QList<KMFilter *> filters;
    for ( int i=0 ; i < numFilters ; ++i ) {
      QString grpName;
      grpName.sprintf("%s #%d", (bPopFilter ? "PopFilter" : "Filter") , i);
      KConfigGroup group = config->group( grpName );
      KMFilter *filter = new KMFilter( group, bPopFilter );
      filter->purify();
      if ( filter->isEmpty() ) {
    #ifndef NDEBUG
        kDebug(5006) << "KMFilter::readConfig: filter\n" << filter->asString()
          << "is empty!";
    #endif
        delete filter;
      } else
        filters.append(filter);
    }
    return filters;
}

/* static */ 
void FilterImporterExporter::writeFiltersToConfig( const QList<KMFilter *> &filters, KConfig *config, bool bPopFilter )
{
    // first, delete all groups:
    QStringList filterGroups =
      config->groupList().grep( QRegExp( bPopFilter ? "PopFilter #\\d+" : "Filter #\\d+" ) );
    foreach ( const QString &s, filterGroups )
      config->deleteGroup( s );
    
    int i = 0;
    for ( QList<KMFilter*>::ConstIterator it = filters.constBegin() ;
          it != filters.constEnd() ; ++it ) {
      if ( !(*it)->isEmpty() ) {
        QString grpName;
        if ( bPopFilter )
          grpName.sprintf("PopFilter #%d", i);
        else
          grpName.sprintf("Filter #%d", i);
        KConfigGroup group = config->group( grpName );
        (*it)->writeConfig( group );
        ++i;
      }
    }
    KConfigGroup group = config->group( "General" );
    if (bPopFilter)
      group.writeEntry("popfilters", i);
    else
      group.writeEntry("filters", i);

    config->sync();
}


FilterImporterExporter::FilterImporterExporter( QWidget* parent, bool popFilter )
 : mParent( parent),
   mPopFilter( popFilter )
{
}

FilterImporterExporter::~FilterImporterExporter()
{
}

QList<KMFilter *> FilterImporterExporter::importFilters()
{
    QString fileName = KFileDialog::getOpenFileName( QDir::homePath(), QString::null, mParent, i18n("Import Filters") );
    if ( fileName.isEmpty() ) 
        return QList<KMFilter *>(); // cancel
    
    { // scoping
        QFile f( fileName );
        if ( !f.open( IO_ReadOnly ) ) {
            KMessageBox::error( mParent, i18n("The selected file is not readable. Your file access permissions might be insufficient.") );
            return QList<KMFilter *>();
        }
    }
    
    KConfig config( fileName );
    QList<KMFilter *> imported = readFiltersFromConfig( &config, mPopFilter );
    FilterSelectionDialog dlg( mParent );
    dlg.setFilters( imported );
    return dlg.exec() == QDialog::Accepted ? dlg.selectedFilters() : QList<KMFilter *>();
}

void FilterImporterExporter::exportFilters(const QList<KMFilter *> &filters )
{
    KUrl saveUrl = KFileDialog::getSaveUrl( QDir::homePath(), QString(), mParent, i18n("Export Filters") );
    
    if ( saveUrl.isEmpty() || !Util::checkOverwrite( saveUrl, mParent ) )
      return;

    KConfig config( saveUrl.toLocalFile() );
    FilterSelectionDialog dlg( mParent );
    dlg.setFilters( filters );
    if ( dlg.exec() == QDialog::Accepted )
        writeFiltersToConfig( dlg.selectedFilters(), &config, mPopFilter );
}

