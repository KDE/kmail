/******************************************************************************
 *
 *  Copyright 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "messagelistview/core/modelinvariantindex.h"
#include "messagelistview/core/modelinvariantrowmapper.h"

namespace KMail
{

namespace MessageListView
{

namespace Core
{

ModelInvariantIndex::~ModelInvariantIndex()
{
  if ( mRowMapper )
    mRowMapper->indexDead( this );
}

int ModelInvariantIndex::currentModelIndexRow()
{
  if ( !mRowMapper )
    return -1;
  return mRowMapper->modelInvariantIndexToModelIndexRow( this );
}

} // namespace Core

} // namespace MessageListView

} // namespace KMail

