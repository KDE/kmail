/*
    This file is part of KMail.
    Copyright (c) 2007 Till Adam <adam@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

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
        filtersListWidget->setAlternatingRowColors( true );
        setMainWidget( filtersListWidget );
        filtersListWidget->setSortingEnabled( false );
        filtersListWidget->setSelectionMode( QAbstractItemView::NoSelection );
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
QList<KMFilter *> FilterImporterExporter::readFiltersFromConfig( KConfig *config,
                                                                 bool bPopFilter )
{
    KConfigGroup group = config->group( "General" );
    int numFilters = 0;
    if ( bPopFilter )
        numFilters = group.readEntry( "popfilters", 0 );
    else
        numFilters = group.readEntry( "filters", 0 );

    QList<KMFilter *> filters;
    for ( int i=0 ; i < numFilters ; ++i ) {
        QString grpName;
        grpName.sprintf( "%s #%d", (bPopFilter ? "PopFilter" : "Filter") , i );
        KConfigGroup group = config->group( grpName );
        KMFilter *filter = new KMFilter( group, bPopFilter );
        filter->purify();
        if ( filter->isEmpty() ) {
    #ifndef NDEBUG
            kDebug(5006) << "Filter" << filter->asString() << "is empty!";
    #endif
            delete filter;
        } else
            filters.append( filter );
    }
    return filters;
}

/* static */
void FilterImporterExporter::writeFiltersToConfig( const QList<KMFilter *> &filters,
                                                   KConfig *config, bool bPopFilter )
{
    // first, delete all filter groups:
    QStringList filterGroups =
      config->groupList().filter( QRegExp( bPopFilter ? "PopFilter #\\d+" : "Filter #\\d+" ) );
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
    QString fileName = KFileDialog::getOpenFileName( QDir::homePath(), QString(),
                                                     mParent, i18n("Import Filters") );
    if ( fileName.isEmpty() )
        return QList<KMFilter *>(); // cancel

    { // scoping
        QFile f( fileName );
        if ( !f.open( IO_ReadOnly ) ) {
            KMessageBox::error( mParent,
                                i18n("The selected file is not readable. "
                                     "Your file access permissions might be insufficient.") );
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
    KUrl saveUrl = KFileDialog::getSaveUrl( QDir::homePath(), QString(),
                                            mParent, i18n("Export Filters") );

    if ( saveUrl.isEmpty() || !Util::checkOverwrite( saveUrl, mParent ) )
        return;

    KConfig config( saveUrl.toLocalFile() );
    FilterSelectionDialog dlg( mParent );
    dlg.setFilters( filters );
    if ( dlg.exec() == QDialog::Accepted )
        writeFiltersToConfig( dlg.selectedFilters(), &config, mPopFilter );
}

