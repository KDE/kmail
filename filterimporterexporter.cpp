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
#include <kdialogbase.h>
#include <klistview.h>

#include <qregexp.h>


using namespace KMail;

class FilterSelectionDialog : public KDialogBase
{
public:
    FilterSelectionDialog( QWidget * parent = 0 )
        :KDialogBase( parent, "filterselection", true, i18n("Select Filters"), Ok|Cancel, Ok, true ),
        wasCancelled( false )
    {
        filtersListView = new KListView( this );
        setMainWidget(filtersListView);
        filtersListView->setSorting( -1 );
        filtersListView->setSelectionMode( QListView::NoSelection );
        filtersListView->addColumn( i18n("Filters"), 300 );
        filtersListView->setFullWidth( true );
        resize( 300, 350 );
    }

    virtual ~FilterSelectionDialog()
    {
    }
    
    virtual void slotCancel()
    {
        wasCancelled = true;
        KDialogBase::slotCancel();
    }
    
    bool cancelled()
    {
        return wasCancelled;
    }

    void setFilters( const QValueList<KMFilter*>& filters )
    {
        originalFilters = filters;
        filtersListView->clear();
        QValueListConstIterator<KMFilter*> it = filters.constEnd();
        while ( it != filters.constBegin() ) {
            --it;
            KMFilter* filter = *it;
            QCheckListItem* item = new QCheckListItem( filtersListView, filter->name(), QCheckListItem::CheckBox );
            item->setOn( true );
        }
    }
    
    QValueList<KMFilter*> selectedFilters() const
    {
        QValueList<KMFilter*> filters;
        QListViewItemIterator it( filtersListView );
        int i = 0;
        while( it.current() ) {
            QCheckListItem* item = static_cast<QCheckListItem*>( it.current() );
            if ( item->isOn() )
                filters << originalFilters[i];
            ++i; ++it;
        }
        return filters;
    }
private:
    KListView *filtersListView;
    QValueList<KMFilter*> originalFilters;
    bool wasCancelled;
};

/* static */
QValueList<KMFilter*> FilterImporterExporter::readFiltersFromConfig( KConfig* config, bool bPopFilter )
{
    KConfigGroupSaver saver(config, "General");
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
    KConfigGroupSaver saver(config, "General");
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
    if ( fileName.isEmpty() ) 
        return QValueList<KMFilter*>(); // cancel
    
    { // scoping
        QFile f( fileName );
        if ( !f.open( IO_ReadOnly ) ) {
            KMessageBox::error( mParent, i18n("The selected file is not readable. Your file access permissions might be insufficient.") );
            return QValueList<KMFilter*>();
        }
    }
    
    KConfig config( fileName );
    QValueList<KMFilter*> imported = readFiltersFromConfig( &config, mPopFilter );
    FilterSelectionDialog dlg( mParent );
    dlg.setFilters( imported );
    dlg.exec();
    return dlg.cancelled() ? QValueList<KMFilter*>() : dlg.selectedFilters();
}

void FilterImporterExporter::exportFilters(const QValueList<KMFilter*> & filters )
{
    KURL saveUrl = KFileDialog::getSaveURL( QDir::homeDirPath(), QString::null, mParent, i18n("Export Filters") );
    
    if ( saveUrl.isEmpty() || !Util::checkOverwrite( saveUrl, mParent ) )
      return;
    
    KConfig config( saveUrl.path() );
    FilterSelectionDialog dlg( mParent );
    dlg.setFilters( filters );
    dlg.exec();
    if ( !dlg.cancelled() )
        writeFiltersToConfig( dlg.selectedFilters(), &config, mPopFilter );
}

