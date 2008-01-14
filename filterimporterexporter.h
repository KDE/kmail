#ifndef __FILTERIMPORTEREXPORTER_H__
#define __FILTERIMPORTEREXPORTER_H__

#include <QList>

class KMFilter;
class KConfig;
class QWidget;

namespace KMail
{

/**
    @short Utility class that provides persisting of filters to/from KConfig.
    @author Till Adam <till@kdab.net>
 */
class FilterImporterExporter
{
public:
      FilterImporterExporter( QWidget *parent, bool popFilter = false );
      virtual ~FilterImporterExporter();
      
      /** Export the given filter rules to a file which 
       * is asked from the user. The list to export is also
       * presented for confirmation/selection. */
      void exportFilters( const QList<KMFilter *> & );
      
      /** Import filters. Ask the user where to import them from
       * and which filters to import. */
      QList<KMFilter *> importFilters();
      
      static void writeFiltersToConfig( const QList<KMFilter *> &filters, KConfig *config, bool bPopFilter );
      static QList<KMFilter *> readFiltersFromConfig( KConfig *config, bool bPopFilter );
private:
      QWidget *mParent;
      bool mPopFilter;
};

}

#endif /* __FILTERIMPORTEREXPORTER_H__ */
