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
      QWidget* mParent;
      bool mPopFilter;
};

}

#endif /* __FILTERIMPORTEREXPORTER_H__ */
