/**
 * linklocator.h
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

#ifndef LINKLOCATOR_H_INCLUDED
#define LINKLOCATOR_H_INCLUDED

#include <qstring.h>

/**
 * LinkLocator assists in identifying sections of text that can
 * usefully be converted in hyperlinks in html. It is intended
 * to be used in two ways: either by calling @ref convertToHtml()
 * to convert a plaintext string into html, or to be derived from
 * where more control is needed.
 *
 * @short Identifies URLs and email addresses embedded in plaintext.
 * @author Dave Corrie <kde@davecorrie.com>
 */
class LinkLocator
{
public:
  /**
   * Constructs a LinkLocator that will search a plaintext string
   * from a given starting point.
   *
   * @param text The string in which to search.
   * @param pos  An index into 'text' from where the search
   *             should begin.
   */
  LinkLocator(const QString& text, int pos = 0);

  /**
   * Sets the maximum length of URLs that will be matched by
   * @ref getUrl(). By default, this is set to 4096
   * characters. The reason for this limit is that there may
   * be possible security implications in handling URLs of
   * unlimited length.
   *
   * @param length The new maximum length of URLs that will be
   *               matched by @ref getUrl().
   */
  void setMaxUrlLen(int length);

  /**
   * @return The current limit on the maximum length of a URL.
   *
   * @see setMaxUrlLen().
   */
  int maxUrlLen() const;

  /**
   * Sets the maximum length of email addresses that will be
   * matched by @ref getEmailAddress(). By default, this is
   * set to 255 characters. The reason for this limit is that
   * there may be possible security implications in handling
   * addresses of unlimited length.
   *
   * @param length The new maximum length of email addresses
   *               that will be matched by @ref getEmailAddress().
   */
  void setMaxAddressLen(int length);

  /**
   * @return The current limit on the maximum length of an email
   *         address.
   *
   * @see setMaxAddressLen().
   */
  int maxAddressLen() const;

  /**
   * Attempts to grab a URL starting at the current scan position.
   * If there is no URL at the current scan position, then an empty
   * string is returned. If a URL is found, the current scan position
   * is set to the index of the last character in the URL.
   *
   * @return The URL at the current scan position, or an empty string.
   */
  QString getUrl();

  /**
   * Attempts to grab an email address. If there is an @ symbol at the
   * current scan position, then the text will be searched both backwards
   * and forwards to find the email address. If there is no @ symbol at
   * the current scan position, an empty string is returned. If an address
   * is found, then the current scan position is set to the index of the
   * last character in the address.
   *
   * @return The email address at the current scan position, or an empty
   *         string.
   */
  QString getEmailAddress();

  /**
   * Converts plaintext into html. The following characters are converted to HTML
   * entities: & " < >. Newlines are also preserved.
   *
   * @param  plainText      The text to be converted into HTML.
   * @param  preserveBlanks Whether to preserve the appearance of sequences of space
   *                        characters and tab characters in the resulting HTML.
   * @param  maxUrlLen      The maximum length of permitted URLs. (See
   *                        @ref maxUrlLen().)
   * @param  maxAddressLen  The maximum length of permitted email addresses.
   *                        (See @ref maxAddressLen().)
   * @return An HTML version of the text supplied in the 'plainText' parameter, suitable
   *         for inclusion in the BODY of an HTML document.
   */
  static QString convertToHtml(const QString& plainText, bool preserveBlanks = false,
    int maxUrlLen = 4096, int maxAddressLen = 255);

protected:
  /**
   * The plaintext string being scanned for URLs and email addresses.
   */
  QString mText;
  /**
   * The current scan position.
   */
  int mPos;

private:
  int mMaxUrlLen;
  int mMaxAddressLen;

  bool atUrl() const;
  bool isEmptyUrl(const QString& url);
  bool isEmptyAddress(const QString& address);
};

#endif // LINKLOCATOR_H_INCLUDED

