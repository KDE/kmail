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
#include <qdialog.h>

class KMFilterActionDict;
class KMGFilterDlg;
class QWidget;
class KConfig;
class KMFolder;
class KMMessage;


#define KMFilterActionInherited QObject
class KMFilterAction: public QObject
{
public:
  /** Initialize filter action with nationalized label and international
   * name. */
  KMFilterAction(const QString name, const QString label);
  virtual ~KMFilterAction();

  /** Returns nationalized label */
  const QString label(void) const { return mLabel; }

  /** Execute action on given message. Returns TRUE if the message
   * shall be processed by further filters and FALSE otherwise. 
   * Set stopIt to TRUE to stop applying filters to this msg and
   * do not change it otherwise. */
  virtual bool process(KMMessage* msg, bool& stopIt) = 0;

  /** Install a setup GUI for other configuration options.
   * There will be no "undo" button, so the GUI elements installed shall
   * directly represent the fields of the filter action. E.g. to modify
   * the label the following call would be issued:
   * caller->addEntry(locale->translate("Label:"), mLabel); */
  virtual void installGUI(KMGFilterDlg* caller) = 0;

  /** Read extra arguments from given string. */
  virtual void argsFromString(const QString argsStr) = 0;

  /** Return extra arguments as string. Must not contain newlines. */
  virtual const QString argsAsString(void) const = 0;

protected:
  QString mLabel;
};


//-----------------------------------------------------------------------------
// Abstract base class for the filter UI. The methods of
// KMFilterAction::installGUI() is allowed to call all the methods given 
// in this class.
#define KMGFilterDlgInherited QDialog

class KMGFilterDlg: public QDialog
{
  Q_OBJECT

public:
  KMGFilterDlg(QWidget* parent=NULL, const char* name=NULL,
	       bool modal=FALSE, WFlags f=0);
  virtual ~KMGFilterDlg();

  /** Add a constant text that cannot be modified. */
  virtual void addLabel(const QString label) = 0;

  /** Add a labeled entry field that allows string input. The given string
      will always contain the current value. */
  virtual void addEntry(const QString label, QString value) = 0;

  /** Add a labeled entry field for filename input with a "..." button that
      opens a file selector with the given pattern/start-dir when clicked. */
  virtual void addFileNameEntry(const QString label, QString value,
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
  KMFilterActionDict();
  ~KMFilterActionDict();
};

#endif /*kmfilteraction_h*/
