/* Mail Filter Action: a filter action has one method that processes the
 * given mail message.
 *
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef kmfilteraction_h
#define kmfilteraction_h

#include <qdict.h>
#include <qobject.h>

class KMFilterActionDict;
class KMFilterActionGUI;
class QWidget;
class KConfig;
class KMFolder;
class KMMessage;


#define KMFilterActionInherited QObject
class KMFilterAction: public QObject
{
public:
  KMFilterAction(const char* label);
  virtual ~KMFilterAction();

  /** Returns nationalized label */
  const char* label(void) const { return name(); }

  /** Execute action on given message. Returns TRUE if the message
      shall be processed by further filter rules and FALSE otherwise. */
  virtual bool process(KMMessage* msg) = 0;

  /** Install a setup GUI for other configuration options.
      There will be no "undo" button, so the GUI elements installed shall
      directly represent the fields of the filter action. E.g. to modify
      the label the following call would be issued:
      caller->addEntry(locale->translate("Label:"), mLabel);
  */
  virtual void installGUI(KMFilterActionGUI* caller) = 0;

  /** Read options from given config file. The config group is pre-set
      and must not be changed. */
  virtual void readConfig(KConfig* config) = 0;

  /** Write options to given config file. The config group is pre-set
      and must not be changed. */
  virtual void writeConfig(KConfig* config) = 0;

};


//-----------------------------------------------------------------------------
// Abstract base class for the filter action GUI. The methods of
// KMFilterAction::installGUI() may call all the methods given in this abstract
// class
class KMFilterActionGUI
{
public:
  /** Add a constant text that cannot be modified. */
  virtual void addLabel(const QString label) = 0;

  /** Add a labeled entry field that allows string input. The given string
      will always contain the current value. */
  virtual void addEntry(const QString label, QString value) = 0;

  /** Add a labeled entry field for filename input with a "..." button that
      opens a file selector with the given pattern/start-dir when clicked. */
  virtual void addEntry(const QString label, QString value,
			const QString pattern, const QString startdir) = 0;

  /** Add a labeled toggle button. */
  virtual void addToggle(const QString label, bool* value) = 0;

  /** Add a labeled list of folders. */
  virtual void addFolderList(const QString label, KMFolder** folderPtr) = 0;

  /** Add the given custom widget with a label to it's left if specified.
      If the label is NULL then the custom widget will have the full width. */
  virtual void addWidget(const QString label, QWidget* widget) = 0;
};


//-----------------------------------------------------------------------------
// Dictionary that contains a list of all registered filter actions
#define KMFilterActionDictInherited QDict<KMFilterAction>
class KMFilterActionDict: public QDict<KMFilterAction>
{
public:
  KMFilterActionDict(int size);
  ~KMFilterActionDict();

  /** Initializes dictionary with standard filter actions */
  void init(void);
};

#endif /*kmfilteraction_h*/
