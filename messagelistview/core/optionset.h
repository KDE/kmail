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

#ifndef __KMAIL_MESSAGELISTVIEW_CORE_OPTIONSET_H__
#define __KMAIL_MESSAGELISTVIEW_CORE_OPTIONSET_H__

#include <QString>

class QDataStream;

namespace KMail
{

namespace MessageListView
{

namespace Core
{

/**
 * A set of options that can be applied to the MessageListView in one shot.
 * In the sources and in the user interface you can find this set of options
 * referred also as "View Mode" or "Preset".
 *
 * The option set has a name and an unique id that identifies it. The name
 * is shown to the user in the combo box above the message list view.
 * The set has also a description that is shown as tooltip and should explain
 * the purpose, the best usage cases, eventually the advantages and disadvantages.
 *
 * The option set can be "packed" to a string and "unpacked" from a string. This
 * is basically for storing it in a configuration file.
 */
class OptionSet
{
public:
  OptionSet();
  OptionSet( const OptionSet &src );
  OptionSet( const QString &name, const QString &description );
  virtual ~OptionSet();

protected:
  QString mId;
  QString mName;
  QString mDescription;

public:
  /**
   * Returns the unique id of this OptionSet.
   * The id can't be set. It's either automatically generated or loaded from configuration.
   */
  const QString &id() const
    { return mId; };

  /**
   * Returns the name of this OptionSet
   */
  const QString &name() const
    { return mName; };

  /**
   * Sets the name of this OptionSet. You must take care
   * of specifying an _unique_ name in order for the Manager to
   * store the sets properly.
   */
  void setName( const QString &name )
    { mName = name; };

  /**
   * Returns a description of this option set. Ideally it should contain
   * its purpose and what to expect from it. But in the end we'll show
   * whatever the user will put in here.
   */
  const QString &description() const
    { return mDescription; };

  /**
   * Sets the description for this option set.
   */
  void setDescription( const QString &description )
    { mDescription = description; };

  /**
   * Packs this configuration object into a string suitable for storing
   * in a config file.
   */
  QString saveToString() const;

  /**
   * Attempts to unpack this configuration object from a string (that is
   * likely to come out from a config file). Returns true if the string
   * was in a valid format and the load operation succeeded, false otherwise.
   */
  bool loadFromString( QString &data );

  /**
   * (Re)generates a (hopefully) unique identifier for this option set.
   * Please note that this function is reserved to this class and to
   * Configure*Dialog instances which need it for cloning option sets.
   * You shouldn't need to call it.
   */
  void generateUniqueId();

protected:
  /**
   * Saves the inner contents of this option set to the specified data stream.
   * The implementation of this method MUST be provided by derived classes.
   */
  virtual void save( QDataStream &s ) const = 0;

  /**
   * Loads the inner contents of this option set from the specified data stream.
   * The implementation of this method MUST be provided by derived classes
   * and must return true in case of success and false in case of load failure.
   */
  virtual bool load( QDataStream &s ) = 0;
};

} // namespace Core

} // namespace MessageListView

} // namespace KMail

#endif //!__KMAIL_MESSAGELISTVIEW_CORE_OPTIONSET_H__
