/* class for drag data 
 * $Id$
 */
#ifndef kmdragdata_h
#define kmdragdata_h

#include <qlist.h>

class Folder;

class KmDragData
{
public:
  init (Folder* fld, int fromId, int toId);

  Folder* folder(void) { return f; }
  int from(void) { return fromId; }
  int to(void) { return toId; }

protected:
  Folder* f;
  int fromId, toId;
};

inline KmDragData::init(Folder* fld, int from, int to)
{
  f = fld;
  fromId = from;
  toId = to;
}

#endif /*kmdragdata_h*/

