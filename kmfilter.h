/* Mail Filter Rule : incoming mail is sent trough the list of mail filter
 * rules before it is placed in the associated mail folder (usually "inbox").
 * This class represents one mail filter rule.
 *
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef kmfilter_h
#define kmfilter_h

#include "kmsearchpattern.h"

#include <qlist.h>

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
  /** Result codes returned by @ref process. They mean:

      @param MsgExpropriated You no longer own the message. E.g. if
      the message was moved to another folder. You should stop
      processing here.

      @param GoOn Everything OK. You are still the owner of the
      message and you should continue applying filter actions to this
      message.

      @param CriticalError A critical error occured (e.g. "disk full").

      @param NoResult For internal use only!

  */
  enum ReturnCode { NoResult, MsgExpropriated, GoOn, CriticalError };

  /** Constructor that initializes from given config file, if given.
    * Filters are stored one by one in config groups, i.e. one filter, one group.
    * The config group has to be preset if config is not NULL. */
  KMFilter( KConfig* aConfig=0 );

  /** Copy constructor. Constructs a deep copy of @p aFilter. */
  KMFilter( KMFilter* aFilter );

  /** Cleanup. */
  virtual ~KMFilter() {};

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
  virtual ReturnCode execActions( KMMessage* msg, bool& stopIt ) const ;

  /** Write contents to given config file. The config group (see the
      constructor above) has to be preset.  The config object will be
      deleted by higher levels, so it is not allowed to store a
      pointer to it anywhere inside this function. */
  virtual void writeConfig( KConfig* config ) const;

  /** Initialize from given config file. The config group (see
      constructor above) has to be preset. The config object will be
      deleted by higher levels, so it is not allowed to store a
      pointer to it anywhere inside this function. */
  virtual void readConfig( KConfig* config );

  /** Remove empty rules (and actions one day). */
  virtual void purify();

  /** Check for empty pattern (and action list one day). */
  virtual bool isEmpty() const
    { return mPattern.isEmpty() || mActions.isEmpty(); }

  /** Provides a reference to the internal action list. If your used
      the @p setAction() and @p action() functions before, please
      convert to using myFilter->actions()->at() and friends now. */
  QList<KMFilterAction>* actions() { return &mActions; }

  /** Provides a reference to the internal pattern. If you used the
      @p matches() function before, please convert to using
      myFilter->pattern()->matches() now. */
  KMSearchPattern* pattern() { return &mPattern; }

  /** 
   * Called from the filter manager when a folder is moved.
   * Tests if the folder aFolder is used in any action. Changes it
   * to aNewFolder folder in this case.
   * @return TRUE if a change in some action occured,
   * FALSE if no action was affected.
   */
  virtual bool folderRemoved( KMFolder* aFolder, KMFolder* aNewFolder );

  /** Returns the filter in a human-readable form. useful for
      debugging but not much else. Don't use, as it may well go away
      in the future... */
  const QString asString() const;

private:
  KMSearchPattern mPattern;
  QList<KMFilterAction> mActions;
};

#endif /*kmfilter_h*/
