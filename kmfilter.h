/* -*- mode: C++; c-file-style: "gnu" -*-
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
#ifndef kmfilter_h
#define kmfilter_h

#include "kmsearchpattern.h"
#include "kmpopheaders.h"

#include <kshortcut.h>

#include <qptrlist.h>

class QString;
class KConfig;
class KMMessage;
class KMFilterAction;
class KMFolder;

// maximum number of filter actions per filter
const int FILTER_MAX_ACTIONS = 8;


class KMFilter
{
public:
  /** Result codes returned by @see process. They mean:

      @param GoOn Everything OK. You are still the owner of the
      message and you should continue applying filter actions to this
      message.

      @param CriticalError A critical error occurred (e.g. "disk full").

      @param NoResult For internal use only!

  */
  enum ReturnCode { NoResult, GoOn, CriticalError };

  /** Constructor that initializes from given config file, if given.
    * Filters are stored one by one in config groups, i.e. one filter, one group.
    * The config group has to be preset if config is not 0. */
  KMFilter( KConfig* aConfig=0 , bool popFilter = false);

  /** Copy constructor. Constructs a deep copy of @p aFilter. */
  KMFilter( const KMFilter & other );

  /** Cleanup. */
  ~KMFilter() {}

  /** Equivalent to @pattern()->name(). @return name of the filter */
  QString name() const {
    return mPattern.name();
  }

  /** Execute the filter action(s) on the given message.
      Returns:
      @li 2 if a critical error occurred,
      @li 1 if the caller is still
      the owner of the message,
      @li 0 if processed successfully.
      @param msg The message to which the actions should be applied.
      @param stopIt Contains
      TRUE if the caller may apply other filters and FALSE if he shall
      stop the filtering of this message.
  */
  ReturnCode execActions( KMMessage* msg, bool& stopIt ) const ;

  /** Determines if the filter depends on the body of the message
  */
  bool requiresBody(KMMsgBase* msgBase);

  /** No descriptions */
  KMPopFilterAction action();

  /** No descriptions */
  void setAction(const KMPopFilterAction aAction);

  /** Write contents to given config file. The config group (see the
      constructor above) has to be preset.  The config object will be
      deleted by higher levels, so it is not allowed to store a
      pointer to it anywhere inside this function. */
  void writeConfig( KConfig* config ) const;

  /** Initialize from given config file. The config group (see
      constructor above) has to be preset. The config object will be
      deleted by higher levels, so it is not allowed to store a
      pointer to it anywhere inside this function. */
  void readConfig( KConfig* config );

  /** Remove empty rules (and actions one day). */
  void purify();

  /** Check for empty pattern and action list. */
  bool isEmpty() const;

  /** Provides a reference to the internal action list. If your used
      the @p setAction() and @p action() functions before, please
      convert to using myFilter->actions()->at() and friends now. */
  QPtrList<KMFilterAction>* actions() { return &mActions; }

  /** Provides a reference to the internal action list. Const version. */
  const QPtrList<KMFilterAction>* actions() const { return &mActions; }

  /** Provides a reference to the internal pattern. If you used the
      @p matches() function before, please convert to using
      myFilter->pattern()->matches() now. */
  KMSearchPattern* pattern() { return &mPattern; }

  /** Provides a reference to the internal pattern. If you used the
      @p matches() function before, please convert to using
      myFilter->pattern()->matches() now. */
  const KMSearchPattern* pattern() const { return &mPattern; }

  /** Set whether this filter should be applied on
      outbound messages (@p aApply == TRUE) or not.
      @see applyOnOutbound applyOnInbound setApplyOnInbound
  */
  void setApplyOnOutbound( bool aApply=TRUE ) { bApplyOnOutbound = aApply; }

  /** @return TRUE if this filter should be applied on
      outbound messages, FALSE otherwise.
      @see setApplyOnOutbound applyOnInbound setApplyOnInbound
  */
  bool applyOnOutbound() const { return bApplyOnOutbound; }

  /** Set whether this filter should be applied on
      inbound messages (@p aApply == TRUE) or not.
      @see setApplyOnOutbound applyOnInbound applyOnOutbound
  */
  void setApplyOnInbound( bool aApply=TRUE ) { bApplyOnInbound = aApply; }

  /** @return TRUE if this filter should be applied on
      inbound messages, FALSE otherwise.
      @see setApplyOnOutbound applyOnOutbound setApplyOnInbound
  */
  bool applyOnInbound() const { return bApplyOnInbound; }

  /** Set whether this filter should be applied on
      explicit (CTRL-J) filtering (@p aApply == TRUE) or not.
      @see setApplyOnOutbound applyOnInbound applyOnOutbound
  */
  void setApplyOnExplicit( bool aApply=TRUE ) { bApplyOnExplicit = aApply; }

  /** @return TRUE if this filter should be applied on
      explicit (CTRL-J) filtering, FALSE otherwise.
      @see setApplyOnOutbound applyOnOutbound setApplyOnInbound
  */
  bool applyOnExplicit() const { return bApplyOnExplicit; }

  void setStopProcessingHere( bool aStop ) { bStopProcessingHere = aStop; }
  bool stopProcessingHere() const { return bStopProcessingHere; }

  /** Set whether this filter should be plugged into the filter menu.
  */
  void setConfigureShortcut( bool aShort ) {
    bConfigureShortcut = aShort;
    bConfigureToolbar = bConfigureToolbar && bConfigureShortcut;
  }

  /** @return TRUE if this filter should be plugged into the filter menu,
      FALSE otherwise.
      @see setConfigureShortcut
  */
  bool configureShortcut() const { return bConfigureShortcut; }

  /** Set whether this filter should be plugged into the toolbar.
      This can be done only if a shortcut is defined.
      @see setConfigureShortcut
  */
  void setConfigureToolbar( bool aTool ) {
    bConfigureToolbar = aTool && bConfigureShortcut;
  }

  /** @return TRUE if this filter should be plugged into the toolbar,
      FALSE otherwise.
      @see setConfigureToolbar
  */
  bool configureToolbar() const { return bConfigureToolbar; }

  /** Set the shortcut to be used if plugged into the filter menu
      or toolbar. Default is no shortcut.
      @see setConfigureShortcut setConfigureToolbar
  */
  void setShortcut( const KShortcut & shortcut ) { mShortcut = shortcut; };

  /** @return The shortcut assigned to the filter.
      @see setShortcut
  */
  const KShortcut & shortcut() const { return mShortcut; }

  /** Set the icon to be used if plugged into the filter menu
      or toolbar. Default is the gear icon.
      @see setConfigureShortcut setConfigureToolbar
  */
  void setIcon( QString icon ) { mIcon = icon; }

  /** @return The name of the icon to be used.
      @see setIcon
  */
  QString icon() const { return mIcon; }

  /**
   * Called from the filter manager when a folder is moved.
   * Tests if the folder aFolder is used in any action. Changes it
   * to aNewFolder folder in this case.
   * @return TRUE if a change in some action occurred,
   * FALSE if no action was affected.
   */
  bool folderRemoved( KMFolder* aFolder, KMFolder* aNewFolder );

  /** Returns the filter in a human-readable form. useful for
      debugging but not much else. Don't use, as it may well go away
      in the future... */
#ifndef NDEBUG
  const QString asString() const;
#endif

  /** No descriptions */
  bool isPopFilter() const {
    return bPopFilter;
  }

  /** Set the mode for using automatic naming for the filter.
      If the feature is enabled, the name is derived from the
      first filter rule.
  */
  void setAutoNaming( bool useAutomaticNames ) {
    bAutoNaming = useAutomaticNames;
  }

  /** @return Tells, if an automatic name is used for the filter
  */
  bool isAutoNaming() const { return bAutoNaming; }

private:
  KMSearchPattern mPattern;
  QPtrList<KMFilterAction> mActions;
  KMPopFilterAction mAction;
  QString mIcon;
  KShortcut mShortcut;
  bool bPopFilter : 1;
  bool bApplyOnInbound : 1;
  bool bApplyOnOutbound : 1;
  bool bApplyOnExplicit : 1;
  bool bStopProcessingHere : 1;
  bool bConfigureShortcut : 1;
  bool bConfigureToolbar : 1;
  bool bAutoNaming : 1;
};

#endif /*kmfilter_h*/
