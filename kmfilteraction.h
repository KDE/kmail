/* Mail Filter Action: a filter action has one method that processes the
 * given mail message.
 *
 * Author: Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef kmfilteraction_h
#define kmfilteraction_h

#include <kmime_mdn.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qdict.h>
#include <qptrlist.h>
#include <qvaluelist.h>
#include <qguardedptr.h>
#include <qobject.h>
#include <qwidget.h>

class KMMessage;
class QWidget;
class KMFolder;
class KTempFile;

//=========================================================
//
// class KMFilterAction
//
//=========================================================


/** Abstract base class for KMail's filter actions. All it can do is
    hold a name (ie. type-string). There are several sub-classes that
    inherit form this and are capable of providing parameter handling
    (import/export as string, a widget to allow editing, etc.)

    @short Abstract base class for KMail's filter actions.
    @author Marc Mutz <Marc@Mutz.com>, based on work by Stefan Taferner <taferner@kde.org>.
    @see KMFilter KMFilterMgr
*/
class KMFilterAction
{
public:
  /** Possible return codes of @ref process:

      @li @p Moved: The message we re-parented.

      @li @p GoOn: Go on with applying filter actions.

      @li @p ErrorButGoOn: There was a non-critical error (e.g. an
      invalid address in the 'forward' action), but the processing
      should continue.

      @li @p CriticalError: A critical error has occured during
      processing (e.g. "disk full").

  */
  enum ReturnCode { Moved = 0x1, GoOn = 0x2, ErrorButGoOn = 0x4,
		    CriticalError = 0x8 };
  /** Initialize filter action with (english) name @p aName and
      (internationalized) label @p aLabel. */
  KMFilterAction(const char* aName, const QString aLabel);
  virtual ~KMFilterAction();

  /** Returns nationalized label, ie. the one which is presented in
      the filter dialog. */
  const QString label() const { return mLabel; }

  /** Returns english name, ie. the one under which it is known in the
      config. */
  const QString name() const { return mName; }

  /** Execute action on given message. Returns @p CriticalError if a
      critical error has occurred (eg. disk full), @p ErrorButGoOn if
      there was a non-critical error (e.g. invalid address in
      'forward' action), @p GoOn if the message shall be processed by
      further filters and @p Ok otherwise.
  */
  virtual ReturnCode process(KMMessage* msg) const = 0;

  /** Determines whether this action is valid. But this is just a
      quick test. Eg., actions that have a mail address as parameter
      shouldn't try real address validation, but only check if the
      string representation is empty. */
  virtual bool isEmpty() const { return FALSE; }

  /** Creates a widget for setting the filter action parameter. Also
      sets the value of the widget. */
  virtual QWidget* createParamWidget(QWidget* parent) const;

  /** The filter action shall set it's parameter from the widget's
      contents. It is allowed that the value is read by the action
      before this function is called. */
  virtual void applyParamWidgetValue(QWidget* paramWidget);

  /** The filter action shall set it's widget's contents from it's
      parameter. */
  virtual void setParamWidgetValue(QWidget* paramWidget) const;

  /** The filter action shall clear it's parameter widget's
      contents. */
  virtual void clearParamWidget(QWidget* paramWidget) const;

  /** Read extra arguments from given string. */
  virtual void argsFromString(const QString argsStr) = 0;

  /** Return extra arguments as string. Must not contain newlines. */
  virtual const QString argsAsString() const = 0;

  /** Called from the filter when a folder is removed.  Tests if the
      folder @p aFolder is used and changes to @p aNewFolder in this
      case. Returns TRUE if a change was made.  */
  virtual bool folderRemoved(KMFolder* aFolder, KMFolder* aNewFolder);

  /** Static function that creates a filter action of this type. */
  static KMFilterAction* newAction();

  /** Temporarily open folder. Will be closed by the next @ref
    KMFilterMgr::cleanup() call.  */
  static int tempOpenFolder(KMFolder* aFolder);

  /** Automates the sending of MDNs from filter actions. */
  static void sendMDN( KMMessage * msg, KMime::MDN::DispositionType d,
		       const QValueList<KMime::MDN::DispositionModifier> & m
		       =QValueList<KMime::MDN::DispositionModifier>() );

private:
  QString mName;
  QString mLabel;
};

//=========================================================
//
// class KMFilterActionWithNone
//
//=========================================================


/** Abstract base class for KMail's filter actions that need no
    parameter, e.g. 'bounce'. Creates an (empty) @ref QWidget as
    parameter widget. A subclass of this must provide at least
    implementations for the following methods:

    @li virtual @ref KMFilterAction::ReturnCodes @ref KMFilterAction::process
    @li static @ref KMFilterAction::newAction

    @short Abstract base class for filter actions with no parameter.
    @author Marc Mutz <Marc@Mutz.com>, based upon work by Stefan Taferner <taferner@kde.org>
    @see KMFilterAction KMFilter

*/
class KMFilterActionWithNone : public KMFilterAction
{
public:
  /** Initialize filter action with (english) name @p aName. This is
      the name under which this action is known in the config file. */
  KMFilterActionWithNone(const char* aName, const QString aLabel);

  /** Read extra arguments from given string. This type of filter
      action has no paramters, so this is a no-op. */
  virtual void argsFromString(const QString) {};

  /** Return extra arguments as string. Must not contain newlines. We
      return @ref QString::null, because we have no parameter. */
  virtual const QString argsAsString() const { return QString::null; }
};

//=========================================================
//
// class KMFilterActionWithString
//
//=========================================================


/** Abstract base class for KMail's filter actions that need a
    free-form parameter, e.g. 'set transport' or 'set reply to'.  Can
    create a @ref QLineEdit as parameter widget. A subclass of this
    must provide at least implementations for the following methods:

    @li virtual @ref KMFilterAction::ReturnCodes @ref KMFilterAction::process
    @li static @ref KMFilterAction::newAction

    @short Abstract base class for filter actions with a free-form string as parameter.
    @author Marc Mutz <Marc@Mutz.com>, based upon work by Stefan Taferner <taferner@kde.org>
    @see KMFilterAction KMFilter

*/
class KMFilterActionWithString : public KMFilterAction
{
public:
  /** Initialize filter action with (english) name @p aName. This is
      the name under which this action is known in the config file. */
  KMFilterActionWithString(const char* aName, const QString aLabel);

  /** Determines whether this action is valid. But this is just a
      quick test. Eg., actions that have a mail address as parameter
      shouldn't try real address validation, but only check if the
      string representation is empty. */
  virtual bool isEmpty() const { return mParameter.stripWhiteSpace().isEmpty(); }

  /** Creates a widget for setting the filter action parameter. Also
      sets the value of the widget. */
  virtual QWidget* createParamWidget(QWidget* parent) const;

  /** The filter action shall set it's parameter from the widget's
      contents. It is allowed that the value is read by the action
      before this function is called. */
  virtual void applyParamWidgetValue(QWidget* paramWidget);

  /** The filter action shall set it's widget's contents from it's
      parameter. */
  virtual void setParamWidgetValue(QWidget* paramWidget) const;

  /** The filter action shall clear it's parameter widget's
      contents. */
  virtual void clearParamWidget(QWidget* paramWidget) const;

  /** Read extra arguments from given string. */
  virtual void argsFromString(const QString argsStr);

  /** Return extra arguments as string. Must not contain newlines. */
  virtual const QString argsAsString() const;

protected:
  QString mParameter;
};

//=========================================================
//
// class KMFilterActionWithUOID
//
//=========================================================


/** Abstract base class for KMail's filter actions that need a
    parameter that has a UOID, e.g. "set identity". A subclass of this
    must provide at least implementations for the following methods:

    @li virtual @ref KMFilterAction::ReturnCodes @ref KMFilterAction::process
    @li static @ref KMFilterAction::newAction
    @li the *ParamWidget* methods.

    @short Abstract base class for filter actions with a free-form string as parameter.
    @author Marc Mutz <Marc@Mutz.com>, based upon work by Stefan Taferner <taferner@kde.org>
    @see KMFilterAction KMFilter

*/
class KMFilterActionWithUOID : public KMFilterAction
{
public:
  /** Initialize filter action with (english) name @p aName. This is
      the name under which this action is known in the config file. */
  KMFilterActionWithUOID(const char* aName, const QString aLabel);

  /** Determines whether this action is valid. But this is just a
      quick test. Eg., actions that have a mail address as parameter
      shouldn't try real address validation, but only check if the
      string representation is empty. */
  virtual bool isEmpty() const { return mParameter == 0; }

  /** Read extra arguments from given string. */
  virtual void argsFromString(const QString argsStr);

  /** Return extra arguments as string. Must not contain newlines. */
  virtual const QString argsAsString() const;

protected:
  uint mParameter;
};

//=========================================================
//
// class KMFilterActionWithStringList
//
//=========================================================


/** Abstract base class for KMail's filter actions that need a
    parameter which can be chosen from a fixed set, e.g. 'set
    identity'.  Can create a @ref QComboBox as parameter widget. A
    subclass of this must provide at least implementations for the
    following methods:

    @li virtual @ref KMFilterAction::ReturnCodes @ref KMFilterAction::process
    @li static @ref KMFilterAction::newAction

    Additionally, it's constructor should populate the @ref
    QStringList @p mParameterList with the valid parameter
    strings. The combobox will then contain be populated automatically
    with those strings. The default string will be the first one.

    @short Abstract base class for filter actions with a fixed set of string parameters.
    @author Marc Mutz <Marc@Mutz.com>, based upon work by Stefan Taferner <taferner@kde.org>
    @see KMFilterActionWithString KMFilterActionWithFolder KMFilterAction KMFilter

*/
class KMFilterActionWithStringList : public KMFilterActionWithString
{
public:
  /** Initialize filter action with (english) name @p aName. This is
      the name under which this action is known in the config file. */
  KMFilterActionWithStringList(const char* aName, const QString aLabel);

  /** Creates a widget for setting the filter action parameter. Also
      sets the value of the widget. */
  virtual QWidget* createParamWidget(QWidget* parent) const;

  /** The filter action shall set it's parameter from the widget's
      contents. It is allowed that the value is read by the action
      before this function is called. */
  virtual void applyParamWidgetValue(QWidget* paramWidget);

  /** The filter action shall set it's widget's contents from it's
      parameter. */
  virtual void setParamWidgetValue(QWidget* paramWidget) const;

  /** The filter action shall clear it's parameter widget's
      contents. */
  virtual void clearParamWidget(QWidget* paramWidget) const;

  /** Read extra arguments from given string. */
  virtual void argsFromString(const QString argsStr);

protected:
  QStringList mParameterList;
};


//=========================================================
//
// class KMFilterActionWithFolder
//
//=========================================================


/** Abstract base class for KMail's filter actions that need a
    mail folder as parameter, e.g. 'move to folder'. Can
    create a @ref QComboBox as parameter widget. A subclass of this
    must provide at least implementations for the following methods:

    @li virtual @ref KMFilterAction::ReturnCodes @ref KMFilterAction::process
    @li static @ref KMFilterAction::newAction

    @short Abstract base class for filter actions with a mail folder as parameter.
    @author Marc Mutz <Marc@Mutz.com>, based upon work by Stefan Taferner <taferner@kde.org>
    @see KMFilterActionWithStringList KMFilterAction KMFilter

*/

class KMFilterActionWithFolder : public KMFilterAction
{
public:
  /** Initialize filter action with (english) name @p aName. This is
      the name under which this action is known in the config file. */
  KMFilterActionWithFolder(const char* aName, const QString aLabel);

  /** Determines whether this action is valid. But this is just a
      quick test. Eg., actions that have a mail address as parameter
      shouldn't try real address validation, but only check if the
      string representation is empty. */
  virtual bool isEmpty() const { return (!mFolder && mFolderName.isEmpty()); }

  /** Creates a widget for setting the filter action parameter. Also
      sets the value of the widget. */
  virtual QWidget* createParamWidget(QWidget* parent) const;

  /** The filter action shall set it's parameter from the widget's
      contents. It is allowed that the value is read by the action
      before this function is called. */
  virtual void applyParamWidgetValue(QWidget* paramWidget);

  /** The filter action shall set it's widget's contents from it's
      parameter. */
  virtual void setParamWidgetValue(QWidget* paramWidget) const;

  /** The filter action shall clear it's parameter widget's
      contents. */
  virtual void clearParamWidget(QWidget* paramWidget) const;

  /** Read extra arguments from given string. */
  virtual void argsFromString(const QString argsStr);

  /** Return extra arguments as string. Must not contain newlines. */
  virtual const QString argsAsString() const;

  /** Called from the filter when a folder is removed.  Tests if the
      folder @p aFolder is used and changes to @p aNewFolder in this
      case. Returns TRUE if a change was made.  */
  virtual bool folderRemoved(KMFolder* aFolder, KMFolder* aNewFolder);

protected:
  QGuardedPtr<KMFolder> mFolder;
  QString mFolderName;
};

//=========================================================
//
// class KMFilterActionWithAddress
//
//=========================================================


/** Abstract base class for KMail's filter actions that need a mail
    address as parameter, e.g. 'forward to'. Can create a @ref
    QComboBox (capable of completion from the address book) as
    parameter widget. A subclass of this must provide at least
    implementations for the following methods:

    @li virtual @ref KMFilterAction::ReturnCodes @ref KMFilterAction::process
    @li static @ref KMFilterAction::newAction

    @short Abstract base class for filter actions with a mail address as parameter.
    @author Marc Mutz <Marc@Mutz.com>, based upon work by Stefan Taferner <taferner@kde.org>
    @see KMFilterActionWithString KMFilterAction KMFilter

*/
class KMFilterActionWithAddress : public KMFilterActionWithString
{
public:
  /** Initialize filter action with (english) name @p aName. This is
      the name under which this action is known in the config file. */
  KMFilterActionWithAddress(const char* aName, const QString aLabel);

  /** Creates a widget for setting the filter action parameter. Also
      sets the value of the widget. */
  virtual QWidget* createParamWidget(QWidget* parent) const;

  /** The filter action shall set it's parameter from the widget's
      contents. It is allowed that the value is read by the action
      before this function is called. */
  virtual void applyParamWidgetValue(QWidget* paramWidget);

  /** The filter action shall set it's widget's contents from it's
      parameter. */
  virtual void setParamWidgetValue(QWidget* paramWidget) const;

  /** The filter action shall clear it's parameter widget's
      contents. */
  virtual void clearParamWidget(QWidget* paramWidget) const;
};

//=========================================================
//
// class KMFilterActionWithCommand
//
//=========================================================


/** Abstract base class for KMail's filter actions that need a command
    line as parameter, e.g. 'forward to'. Can create a @ref QLineEdit
    (are there better widgets in the depths of the kdelibs?) as
    parameter widget. A subclass of this must provide at least
    implementations for the following methods:

    @li virtual @ref KMFilterAction::ReturnCodes @ref KMFilterAction::process
    @li static @ref KMFilterAction::newAction

    The implementation of @ref KMFilterAction::process should take the
    command line specified in mParameter, make all required
    modifications and stream the resulting command line into @p
    mProcess. Then you can start the command with @p mProcess.start().

    @short Abstract base class for filter actions with a command line as parameter.
    @author Marc Mutz <Marc@Mutz.com>, based upon work by Stefan Taferner <taferner@kde.org>
    @see KMFilterActionWithString KMFilterAction KMFilter KProcess

*/
class KMFilterActionWithUrl : public KMFilterAction
{
public:
  /** Initialize filter action with (english) name @p aName. This is
      the name under which this action is known in the config file. */
    KMFilterActionWithUrl(const char* aName, const QString aLabel);
    ~KMFilterActionWithUrl();
  /** Determines whether this action is valid. But this is just a
      quick test. Eg., actions that have a mail address as parameter
      shouldn't try real address validation, but only check if the
      string representation is empty. */
  virtual bool isEmpty() const { return mParameter.stripWhiteSpace().isEmpty(); }

  /** Creates a widget for setting the filter action parameter. Also
      sets the value of the widget. */
  virtual QWidget* createParamWidget(QWidget* parent) const;

  /** The filter action shall set it's parameter from the widget's
      contents. It is allowed that the value is read by the action
      before this function is called. */
  virtual void applyParamWidgetValue(QWidget* paramWidget);

  /** The filter action shall set it's widget's contents from it's
      parameter. */
  virtual void setParamWidgetValue(QWidget* paramWidget) const;

  /** The filter action shall clear it's parameter widget's
      contents. */
  virtual void clearParamWidget(QWidget* paramWidget) const;

  /** Read extra arguments from given string. */
  virtual void argsFromString(const QString argsStr);

  /** Return extra arguments as string. Must not contain newlines. */
  virtual const QString argsAsString() const;

protected:
  QString mParameter;
};


class KMFilterActionWithCommand : public KMFilterActionWithUrl
{
public:
  /** Initialize filter action with (english) name @p aName. This is
      the name under which this action is known in the config file. */
  KMFilterActionWithCommand(const char* aName, const QString aLabel);

  /** Creates a widget for setting the filter action parameter. Also
      sets the value of the widget. */
  virtual QWidget* createParamWidget(QWidget* parent) const;

  /** The filter action shall set it's parameter from the widget's
      contents. It is allowed that the value is read by the action
      before this function is called. */
  virtual void applyParamWidgetValue(QWidget* paramWidget);

  /** The filter action shall set it's widget's contents from it's
      parameter. */
  virtual void setParamWidgetValue(QWidget* paramWidget) const;

  /** The filter action shall clear it's parameter widget's
      contents. */
  virtual void clearParamWidget(QWidget* paramWidget) const;

  /** Substitutes various placeholders for data from the message
      resp. for filenames containing that data. Currently, only %n is
      supported, where n in an integer >= 0. %n gets substituted for
      the name of a tempfile holding the n'th message part, with n=0
      meaning the body of the message. */
  virtual QString substituteCommandLineArgsFor( KMMessage *aMsg, QPtrList<KTempFile> & aTempFileList  ) const;

  virtual ReturnCode genericProcess( KMMessage * aMsg, bool filtering ) const;
};



class KMFilterActionWithTest : public KMFilterAction
{
public:
  /** Initialize filter action with (english) name @p aName. This is
      the name under which this action is known in the config file. */
  KMFilterActionWithTest(const char* aName, const QString aLabel);
    ~KMFilterActionWithTest();
  /** Determines whether this action is valid. But this is just a
      quick test. Eg., actions that have a mail address as parameter
      shouldn't try real address validation, but only check if the
      string representation is empty. */
  virtual bool isEmpty() const { return mParameter.stripWhiteSpace().isEmpty(); }

  /** Creates a widget for setting the filter action parameter. Also
      sets the value of the widget. */
  virtual QWidget* createParamWidget(QWidget* parent) const;

  /** The filter action shall set it's parameter from the widget's
      contents. It is allowed that the value is read by the action
      before this function is called. */
  virtual void applyParamWidgetValue(QWidget* paramWidget);

  /** The filter action shall set it's widget's contents from it's
      parameter. */
  virtual void setParamWidgetValue(QWidget* paramWidget) const;

  /** The filter action shall clear it's parameter widget's
      contents. */
  virtual void clearParamWidget(QWidget* paramWidget) const;

  /** Read extra arguments from given string. */
  virtual void argsFromString(const QString argsStr);

  /** Return extra arguments as string. Must not contain newlines. */
  virtual const QString argsAsString() const;

protected:
  QString mParameter;
};


typedef KMFilterAction* (*KMFilterActionNewFunc)(void);


//-----------------------------------------------------------------------------
/** Auxiliary struct to @ref KMFilterActionDict.  */
struct KMFilterActionDesc
{
  QString label, name;
  KMFilterActionNewFunc create;
};

/** Dictionary that contains a list of all registered filter actions
    with their creation functions. They are hard-coded into the
    constructor. If you want to add a new @ref KMFilterAction, make
    sure you add the details of it in @ref init, too.

    You will be able to find a description of a KMFilterAction by
    looking up either it's (english) name or it's (i18n) label:
    <pre>
    KMFilterActionDict dict;
    // get name of the action with label "move to folder":
    dict[i18n("move to folder")]->name; // == "transfer"
    // create one such action:
    KMFilterAction *action = dict["transfer"]->create();
    </pre>

    You can iterate over all known filter actions by using @ref list.

    @short List of known KMFilterAction-types.
    @author Marc Mutz <Marc@Mutz.com>, based on work by Stefan Taferner <taferner@kde.org>
    @see KMFilterAction KMFilterActionDesc KMFilter

*/
class KMFilterActionDict: public QDict<KMFilterActionDesc>
{
public:
  KMFilterActionDict();

  /** Overloaded member function, provided for convenience. Thin
      wrapper around @ref QDict::insert and @ref
      QPtrList::insert. Inserts the resulting @ref KMFilterActionDesc
      thrice: First with the name, then with the label as key into the
      @ref @QDict, then into the @ref QPtrList. For that, it creates an
      instance of the action internally and deletes it again after
      querying it for name and label. */
  void insert(KMFilterActionNewFunc aNewFunc);

  /** Provides read-only access to a list of all known filter
      actions. */
  const QPtrList<KMFilterActionDesc>& list() const { return mList; }

protected:
  /** Populate the dictionary with all known @ref KMFilterAction
     types. Called automatically from the constructor. */
  virtual void init(void);

private:
  QPtrList<KMFilterActionDesc> mList;
};

#endif /*kmfilteraction_h*/
