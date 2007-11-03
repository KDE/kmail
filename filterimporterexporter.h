#ifndef __FILTERIMPORTEREXPORTER_H__
#define __FILTERIMPORTEREXPORTER_H__

#include <qvaluelist.h>

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
      FilterImporterExporter( QWidget* parent, bool popFilter = false );
      virtual ~FilterImporterExporter();
      
      /** Export the given filter rules to a file which 
       * is asked from the user. The list to export is also
       * presented for confirmation/selection. */
      void exportFilters( const QValueList<KMFilter*> & );
      
      /** Import filters. Ask the user where to import them from
       * and which filters to import. */
      QValueList<KMFilter*> importFilters();
      
      static void writeFiltersToConfig( const QValueList<KMFilter*>& filters, KConfig* config, bool bPopFilter );
      static QValueList<KMFilter*> readFiltersFromConfig( KConfig* config, bool bPopFilter );
private:
      bool mPopFilter;
      QWidget* mParent;
};

}

#endif /* __FILTERIMPORTEREXPORTER_H__ */
