/**
 * spellingfilter.cpp
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

#include <kdebug.h>
#include "spellingfilter.h"

//-----------------------------------------------------------------------------
// SpellingFilter implementation
//

SpellingFilter::SpellingFilter(const QString& text, const QString& quotePrefix,
  UrlFiltering filterUrls, EmailAddressFiltering filterEmailAddresses,
  const QStringList& filterStrings)
  : mOriginal(text)
{
  TextCensor c(text);

  if(!quotePrefix.isEmpty())
    c.censorQuotations(quotePrefix);

  if(filterUrls)
    c.censorUrls();

  if(filterEmailAddresses)
    c.censorEmailAddresses();

  QStringList::const_iterator iter = filterStrings.begin();
  while(iter != filterStrings.end())
  {
    c.censorString(*iter);
    ++iter;
  }

  mFiltered = c.censoredText();
}

QString SpellingFilter::originalText() const
{
  return mOriginal;
}

QString SpellingFilter::filteredText() const
{
  return mFiltered;
}

//-----------------------------------------------------------------------------
// SpellingFilter::TextCensor implementation
//

SpellingFilter::TextCensor::TextCensor(const QString& s)
  : LinkLocator(s)
{

}

void SpellingFilter::TextCensor::censorQuotations(const QString& quotePrefix)
{
  mPos = 0;
  while(mPos < static_cast<int>(mText.length()))
  {
    // Find start of quotation
    findQuotation(quotePrefix);
    if(mPos < static_cast<int>(mText.length()))
    {
      int start = mPos;
      skipQuotation(quotePrefix);

      // Replace quotation with spaces
      int len = mPos - start;
      QString spaces;
      spaces.fill(' ', len);
      mText.replace(start, len, spaces);

      //kdDebug(5006) << "censored quotation ["
      //  << start << ", " << mPos << ")" << endl;
    }
  }
}

void SpellingFilter::TextCensor::censorUrls()
{
  mPos = 0;
  while(mPos < static_cast<int>(mText.length()))
  {
    // Find start of url
    QString url;
    while(mPos < static_cast<int>(mText.length()) && url.isEmpty())
    {
      url = getUrl();
      ++mPos;
    }

    if(mPos < static_cast<int>(mText.length()) && !url.isEmpty())
    {
      int start = mPos - url.length();

      // Replace url with spaces
      url.fill(' ');
      mText.replace(start, url.length(), url);

      //kdDebug(5006) << "censored url ["
      //  << start << ", " << mPos << ")" << endl;
    }
  }
}

void SpellingFilter::TextCensor::censorEmailAddresses()
{
  mPos = 0;
  while(mPos < static_cast<int>(mText.length()))
  {
    // Find start of email address
    findEmailAddress();
    if(mPos < static_cast<int>(mText.length()))
    {
      QString address = getEmailAddress();
      ++mPos;
      if(!address.isEmpty())
      {
        int start = mPos - address.length();

        // Replace address with spaces
        address.fill(' ');
        mText.replace(start, address.length(), address);

        //kdDebug(5006) << "censored addr ["
        //  << start << ", "<< mPos << ")" << endl;
      }
    }
  }
}

void SpellingFilter::TextCensor::censorString(const QString& s)
{
  mPos = 0;
  while(mPos != -1)
  {
    // Find start of string
    mPos = mText.find(s, mPos);
    if(mPos != -1)
    {
      // Replace string with spaces
      QString spaces;
      spaces.fill(' ', s.length());
      mText.replace(mPos, s.length(), spaces);
      mPos += s.length();

      //kdDebug(5006) << "censored string ["
      //  << mPos << ", "<< mPos+s.length() << ")" << endl;
    }
  }
}

QString SpellingFilter::TextCensor::censoredText() const
{
  return mText;
}

//-----------------------------------------------------------------------------
// text censorship helper functions
//

bool SpellingFilter::TextCensor::atLineStart() const
{
  return (mPos == 0 && static_cast<int>(mText.length()) > 0) || (mText[mPos - 1] == '\n');
}

void SpellingFilter::TextCensor::skipLine()
{
  mPos = mText.find('\n', mPos);
  if(mPos == -1)
    mPos = static_cast<int>(mText.length());
  else
    ++mPos;
}

bool SpellingFilter::TextCensor::atQuotation(const QString& quotePrefix) const
{
  return atLineStart() &&
    mText.mid(mPos, quotePrefix.length()) == quotePrefix;
}

void SpellingFilter::TextCensor::skipQuotation(const QString& quotePrefix)
{
  while(atQuotation(quotePrefix))
    skipLine();
}

void SpellingFilter::TextCensor::findQuotation(const QString& quotePrefix)
{
  while(mPos < static_cast<int>(mText.length()) && !atQuotation(quotePrefix))
    skipLine();
}

void SpellingFilter::TextCensor::findEmailAddress()
{
  while(mPos < static_cast<int>(mText.length()) && mText[mPos] != '@')
    ++mPos;
}

