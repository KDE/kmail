/*
 * kfontutils implementation
 *
 * Copyright (C) 1998 Stefan Taferner
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABLILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details. You should have received a copy
 * of the GNU General Public License along with this program; if not, write
 * to the Free Software Foundation, Inc, 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 */

#include "kfontutils.h"
#include <qstring.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static QFont::CharSet encIso8859[] = 
{
  QFont::AnyCharSet, QFont::Latin1,  QFont::Latin2,
  QFont::Latin3,  QFont::Latin4,  QFont::Latin5, 
  QFont::Latin6,  QFont::Latin7,  QFont::Latin8, 
  QFont::Latin9, (QFont::CharSet)-1
};
static char fontStr[256];


//-----------------------------------------------------------------------------
const char* kfontToStr(const QFont& aFont)
{
  char *weightStr, *slantStr, *charsetStr;
  int enc, i;
  QFont::Weight weight;
  QFont::CharSet charset;
  
  weight = (QFont::Weight)aFont.weight();
  if (weight == QFont::Light) weightStr = "light";
  else if (weight == QFont::DemiBold) weightStr = "demibold";
  else if (weight == QFont::Bold) weightStr = "bold";
  else if (weight == QFont::Black) weightStr = "black";
  else weightStr = "normal";

  slantStr = aFont.italic() ? "i" : "r";

  charset = aFont.charSet();
  if (charset == QFont::AnyCharSet)
  {
    charsetStr = "*";
    enc = 0;
  }
  else 
  {
    charsetStr = "iso8859";
    for (i=0; (int)encIso8859[i] >= 0; i++)
      if (encIso8859[i] == charset) enc=i;
  }

  sprintf(fontStr, "%s-%s-%s-%d-%s-%d", aFont.family(), weightStr, slantStr,
	  aFont.pointSize(), charsetStr, enc);

  return fontStr;
}


//-----------------------------------------------------------------------------
const QFont kstrToFont(const char* aStr)
{
  char* pos;
  QFont font;
  QFont::CharSet charset;
  int i;

  strncpy(fontStr, aStr, 255);
  fontStr[255] = '\0';

  if (!(pos=strtok(fontStr,"-"))) return font;
  font.setFamily(pos);

  if (!(pos=strtok(NULL,"-"))) return font;
  if (stricmp(pos,"light")==0) font.setWeight(QFont::Light);
  else if (stricmp(pos,"demibold")==0) font.setWeight(QFont::DemiBold);
  else if (stricmp(pos,"bold")==0) font.setWeight(QFont::Bold);
  else if (stricmp(pos,"black")==0) font.setWeight(QFont::Black);
  else font.setWeight(QFont::Normal);

  if (!(pos=strtok(NULL,"-"))) return font;
  if (pos[0]=='i' || pos[0]=='o') font.setItalic(TRUE);

  if (!(pos=strtok(NULL,"-"))) return font;
  font.setPointSize(atoi(pos));

  if (!(pos=strtok(NULL,"-"))) return font;
  if (stricmp(pos,"iso8859")==0)
  {
    pos=strtok(NULL,"-");
    i = pos ? atoi(pos) : 0;
    if (i < 0 || i > 9) i = 0;
    charset = encIso8859[i];
  }
  else charset = QFont::AnyCharSet;
  font.setCharSet(charset);

  return font;
}
