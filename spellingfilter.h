/**
 * spellingfilter.h
 *
 * Copyright (c) 2002 Dave Corrie <kde@davecorrie.com>
 *
 *  This file is part of KMail.
 *
 *  KMail is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SPELLINGFILTER_H_INCLUDED
#define SPELLINGFILTER_H_INCLUDED

#include <qstring.h>
#include <qstringlist.h>
#include "linklocator.h"

class SpellingFilter
{
public:
  enum UrlFiltering { DontFilterUrls, FilterUrls };
  enum EmailAddressFiltering { DontFilterEmailAddresses, FilterEmailAddresses };

  SpellingFilter(const QString& text, const QString& quotePrefix,
    UrlFiltering filterUrls = FilterUrls,
    EmailAddressFiltering filterEmailAddresses = FilterEmailAddresses,
    const QStringList& filterStrings = QStringList());

  QString originalText() const;
  QString filteredText() const;

  class TextCensor;

private:
  const QString mOriginal;
  QString mFiltered;
};

class SpellingFilter::TextCensor : public LinkLocator
{
public:
  TextCensor(const QString& s);

  void censorQuotations(const QString& quotePrefix);
  void censorUrls();
  void censorEmailAddresses();
  void censorString(const QString& s);

  QString censoredText() const;

private:
  bool atLineStart() const;
  void skipLine();

  bool atQuotation(const QString& quotePrefix) const;
  void skipQuotation(const QString& quotePrefix);
  void findQuotation(const QString& quotePrefix);

  void findEmailAddress();
};

#endif // SPELLINGFILTER_H_INCLUDED

