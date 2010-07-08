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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifndef kmfilter_h
#define kmfilter_h

#include "kmsearchpattern.h"

#include <kshortcut.h>
#include <akonadi/collection.h>

#include <kmime/kmime_message.h>

class QString;
class KConfigGroup;
class KMFilterAction;

// maximum number of filter actions per filter
const int FILTER_MAX_ACTIONS = 8;

//Keep these corresponding to the column numbers in the dialog for easier coding
//or change mapToAction and mapToColumn in PopHeadersView
enum KMPopFilterAction { Down=0, Later=1, Delete=2, NoAction=3 };


class KMFilter
{
public:
  /** Result codes returned by process. They mean:

      @param GoOn Everything OK. You are still the owner of the
      message and you should continue applying filter actions to this
      message.

      @param CriticalError A critical error occurred (e.g. "disk full").

      @param NoResult For internal use only!

  */
  enum ReturnCode { NoResult, GoOn, CriticalError };

  /** Account type codes used by setApplicability. They mean:

      @param All Apply to all accounts

      @param ButImap Apply to all but IMAP accounts

      @param Checked apply to all accounts specified by setApplyOnAccount

  */
  enum AccountType { All, ButImap, Checked };

  /** Constructor that initializes basic settings. */
  KMFilter( bool popFilter = false);

  /** Constructor that initializes from given config group.
    * Filters are stored one by one in config groups, i.e.
    * one filter, one group. */
  explicit KMFilter( KConfigGroup & aConfig, bool popFilter = false);

  /** Copy constructor. Constructs a deep copy of @p aFilter. */
  KMFilter( const KMFilter & other );

  /** Cleanup. */
  ~KMFilter();

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
      @param item The message to which the actions should be applied.
      @param stopIt Contains
      true if the caller may apply other filters and false if he shall
      stop the filtering of this message.
  */
  ReturnCode execActions( const Akonadi::Item &item, bool& stopIt ) const ;

  /** Determines if the filter depends on the body of the message
  */
  bool requiresBody();

  /** No descriptions */
  KMPopFilterAction action();

  /** No descriptions */
  void setAction(const KMPopFilterAction aAction);

  /** Write contents to given config group. */
  void writeConfig( KConfigGroup& config ) const;

  /** Initialize from given config group. */
  void readConfig( KConfigGroup& config );

  /** Remove empty rules (and actions one day). */
  void purify();

  /** Check for empty pattern and action list. */
  bool isEmpty() const;

  /** Provides a reference to the internal action list. If your used
      the @p setAction() and @p action() functions before, please
      convert to using myFilter->actions()->at() and friends now. */
  QList<KMFilterAction*>* actions() { return &mActions; }

  /** Provides a reference to the internal action list. Const version. */
  const QList<KMFilterAction*>* actions() const { return &mActions; }

  /** Provides a reference to the internal pattern. If you used the
      @p matches() function before, please convert to using
      myFilter->pattern()->matches() now. */
  KMSearchPattern* pattern() { return &mPattern; }

  /** Provides a reference to the internal pattern. If you used the
      @p matches() function before, please convert to using
      myFilter->pattern()->matches() now. */
  const KMSearchPattern* pattern() const { return &mPattern; }

  /** Set whether this filter should be applied on
      outbound messages (@p aApply == true) or not.
      See applyOnOutbound applyOnInbound setApplyOnInbound
  */
  void setApplyOnOutbound( bool aApply=true ) { bApplyOnOutbound = aApply; }

  /** Set whether this filter should be applied on
      outbound messages before sending (@p aApply == TRUE) or not.
      See applyOnOutbound applyOnInbound setApplyOnInbound
  */
  void setApplyBeforeOutbound( bool aApply=true ) { bApplyBeforeOutbound = aApply; }

  /** @return true if this filter should be applied on
      outbound messages, false otherwise.
      @see setApplyOnOutbound applyOnInbound setApplyOnInbound
  */
  bool applyOnOutbound() const { return bApplyOnOutbound; }

  /** @return TRUE if this filter should be applied on
      outbound messages before they are sent, FALSE otherwise.
      @see setApplyOnOutbound applyOnInbound setApplyOnInbound
  */
  bool applyBeforeOutbound() const { return bApplyBeforeOutbound; }

  /** Set whether this filter should be applied on
      inbound messages (@p aApply == true) or not.
      @see setApplyOnOutbound applyOnInbound applyOnOutbound
  */
  void setApplyOnInbound( bool aApply=true ) { bApplyOnInbound = aApply; }

  /** @return true if this filter should be applied on
      inbound messages, false otherwise.
      @see setApplyOnOutbound applyOnOutbound setApplyOnInbound
  */
  bool applyOnInbound() const { return bApplyOnInbound; }

  /** Set whether this filter should be applied on
      explicit (CTRL-J) filtering (@p aApply == true) or not.
      @see setApplyOnOutbound applyOnInbound applyOnOutbound
  */
  void setApplyOnExplicit( bool aApply=true ) { bApplyOnExplicit = aApply; }

  /** @return true if this filter should be applied on
      explicit (CTRL-J) filtering, false otherwise.
      @see setApplyOnOutbound applyOnOutbound setApplyOnInbound
  */
  bool applyOnExplicit() const { return bApplyOnExplicit; }

  /** Set whether this filter should be applied on
      inbound messages for all accounts (@p aApply == All) or
      inbound messages for all but IMAP accounts (@p aApply == ButImap) or
      for a specified set of accounts only.
      Only applicable to filters that are applied on inbound messages.
      @see setApplyOnInbound setApplyOnAccount
  */
  void setApplicability( AccountType aApply=All ) { mApplicability = aApply; }

  /** @return true if this filter should be applied on
      inbound messages for all accounts, or false if this filter
      is to be applied on a specified set of accounts only.
      Only applicable to filters that are applied on inbound messages.
      @see setApplicability
  */
  AccountType applicability() const { return mApplicability; }

  /** Set whether this filter should be applied on
      inbound messages for the account with id (@p id).
      Only applicable to filters that are only applied to a specified
      set of accounts.
      @see setApplicability applyOnAccount
  */
  void setApplyOnAccount( const QString& id, bool aApply=true );

  /** @return true if this filter should be applied on
      inbound messages from the account with id (@p id), false otherwise.
      @see setApplicability
  */
  bool applyOnAccount( const QString& id ) const;

  void setStopProcessingHere( bool aStop ) { bStopProcessingHere = aStop; }
  bool stopProcessingHere() const { return bStopProcessingHere; }

  /** Set whether this filter should be plugged into the filter menu.
  */
  void setConfigureShortcut( bool aShort ) {
    bConfigureShortcut = aShort;
    bConfigureToolbar = bConfigureToolbar && bConfigureShortcut;
  }

  /** @return true if this filter should be plugged into the filter menu,
      false otherwise.
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

  /** @return true if this filter should be plugged into the toolbar,
      false otherwise.
      @see setConfigureToolbar
  */
  bool configureToolbar() const { return bConfigureToolbar; }

  /** @return The toolbar name of this filter.
   *  @see setToolbarName
   */
  QString toolbarName() const;

  /** This sets the toolbar name for this filter.
   *  The toolbar name is the text to be displayed underneath the toolbar icon
   *  for this filter. This is usually the same as name(),  expect when
   *  explicitly set by this function.
   *  This is useful if the normal filter mame is too long for the toolbar.
   *  @see toolbarName, name
   */
  void setToolbarName( QString toolbarName ) {
    mToolbarName = toolbarName;
  }

  /** Set the shortcut to be used if plugged into the filter menu
      or toolbar. Default is no shortcut.
      @see setConfigureShortcut setConfigureToolbar
  */
  void setShortcut( const KShortcut & shortcut ) { mShortcut = shortcut; }

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
   * @return true if a change in some action occurred,
   * false if no action was affected.
   */
  bool folderRemoved( const Akonadi::Collection& aFolder, const Akonadi::Collection& aNewFolder );

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
  QList<KMFilterAction*> mActions;
  QStringList mAccounts;
  KMPopFilterAction mAction;
  QString mIcon;
  QString mToolbarName;
  KShortcut mShortcut;
  bool bPopFilter : 1;
  bool bApplyOnInbound : 1;
  bool bApplyBeforeOutbound : 1;
  bool bApplyOnOutbound : 1;
  bool bApplyOnExplicit : 1;
  bool bStopProcessingHere : 1;
  bool bConfigureShortcut : 1;
  bool bConfigureToolbar : 1;
  bool bAutoNaming : 1;
  AccountType mApplicability;
};

#endif /*kmfilter_h*/
