/**
 *
 * Copyright (c) 2006 Till Adam <adam@kde.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */

#include "collectionquotapage_p.h"

#include <qlayout.h>
#include <qlabel.h>
#include <qprogressbar.h>
#include <qwhatsthis.h>
#include <qcombobox.h>

#include <math.h>

#include "kmkernel.h"
#include "klocale.h"
#include "kconfig.h"
#include "kdebug.h"
#include "kdialog.h"
#include "globalsettings.h"


QuotaWidget::QuotaWidget( QWidget* parent )
        :QWidget( parent )
{
      QVBoxLayout *box = new QVBoxLayout( this );
      QWidget *stuff = new QWidget( this );
      QGridLayout* layout = new QGridLayout( stuff );
      layout->setMargin( KDialog::marginHint() );
      layout->setSpacing( KDialog::spacingHint() );
      mProgressBar = new QProgressBar( stuff );
      layout->addWidget( mProgressBar, 2, 1 );
      box->addWidget( stuff );
      box->addStretch( 2 );
}

void QuotaWidget::setQuotaInfo( qint64 current, qint64 maxValue )
{
  mProgressBar->setMaximum( maxValue );
  mProgressBar->setValue( current );
}

#include "collectionquotapage_p.moc"
