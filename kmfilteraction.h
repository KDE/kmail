/* Mail Filter Action: a filter action has one method that processes the
 * given mail message.
 *
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef kmfilteraction_h
#define kmfilteraction_h

#include <qlist.h>
#include <qobject.h>
#include <qdialog.h>

class KMFilterActionDict;
class KMGFilterDlg;
class QWidget;
class QPushButton;
class QComboBox;
class QLineEdit;
class KConfig;
class KMFolder;
class KMMessage;


#define KMFilterActionInherited QObject
class KMFilterAction: public QObject
{
public:
  /** Initialize filter action with international
   * name. */
  KMFilterAction(const char* name);
  virtual ~KMFilterAction();

  /** Returns nationalized label */
  virtual const QString label(void) const = 0;

  /** Execute action on given message. Returns TRUE if the message
   * shall be processed by further filters and FALSE otherwise. 
   * Set stopIt to TRUE to stop applying filters to this msg and
   * do not change it otherwise. */
  virtual bool process(KMMessage* msg, bool& stopIt) = 0;

  /** Creates a widget for setting the filter action parameter. Also
   * sets the value of the widget. */
  virtual QWidget* createParamWidget(KMGFilterDlg* parent);

  /** The filter-action shall set it's parameter from the widget's
   * contents. It is allowed that the value is read by the action
   * before this function is called. */
  virtual void applyParamWidgetValue(QWidget* paramWidget);

  /** Read extra arguments from given string. */
  virtual void argsFromString(const QString argsStr) = 0;

  /** Return extra arguments as string. Must not contain newlines. */
  virtual const QString argsAsString(void) const = 0;

  /** Static function that creates a filter action of this type. */
  static KMFilterAction* newAction(void);
};

typedef KMFilterAction* (*KMFilterActionNewFunc)(void);


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

  /** Creates a details button "..." for the current filter action. */
  virtual QPushButton* createDetailsButton(void) = 0;

  /** Sets the label for the current fiter action parameter. */
  //virtual void setLabel(const QString label) = 0;

  /** Creates a combobox with a list of folders for the current filter 
    action, with curFolder as the current entry (if given). */
  virtual QComboBox* createFolderCombo(const QString curFolder=NULL) = 0;
};


//-----------------------------------------------------------------------------
// Dictionary that contains a list of all registered filter actions with
// their creation functions.
class KMFilterActionDesc
{
public:
  QString label, name;
  KMFilterActionNewFunc func;
};
typedef QList<KMFilterActionDesc> KMFilterActionDescList;

class KMFilterActionDict
{
public:
  KMFilterActionDict();
  virtual ~KMFilterActionDict();

  virtual void insert(const QString name, const QString label,
		      KMFilterActionNewFunc func);

  // returns name of element or an empty QString if end of list.
  virtual const QString first(void);
  virtual const QString next(void);

  // these methods work with the current element of first/next
  virtual const QString currentName(void);
  virtual const QString currentLabel(void);
  virtual KMFilterAction* currentCreate(void);

  // Note: the following methods use first/next internally
  virtual KMFilterAction* create(const QString name);
  virtual const QString labelOf(const QString name);
  virtual const QString nameOf(const QString label);

protected:
  virtual void init(void);

  // Find filter action with given name
  virtual KMFilterActionDesc* find(const QString name);

  KMFilterActionDescList mList;
};

#endif /*kmfilteraction_h*/
