// kmfilterrulesedit.h
// Author: Marc Mutz <Marc@Mutz.com>
// This code is under GPL

#ifndef KMFILTERRULESEDIT_H
#define KMFILTERRULESEDIT_H

#include "kmsearchpattern.h"
#include "kwidgetlister.h"

#include <qhbox.h>
#include <qgroupbox.h>
#include <qptrlist.h>
#include <qstringlist.h>

class QPushButton;
class QVBoxLayout;
class KMSearchRule;
class QString;
class QComboBox;
class QLineEdit;
class QRadioButton;
class QPushButton;
class KButtonBox;
class QVBoxLayout;

/** A widget to edit a single KMSearchRule.
    It consists of an editable @ref QComboBox for the field,
    a read-only @ref QComboBox for the function and
    a @ref QLineEdit for the content or the pattern (in case of regexps).
    It manages the i18n itself, so field name should be in it's english form.

    To use, you essentially give it the reference to a @ref
    KMSearchRule and it does the rest. It will never delete the rule
    itself, as it assumes that something outside of it manages this.

    @short A widget to edit a single KMSearchRule.
    @author Marc Mutz <Marc@Mutz.com>
*/

class KMSearchRuleWidget: public QHBox
{
  Q_OBJECT
public:
  /** Constructor. You can give a @ref KMSearchRule as parameter, which will
      be used to initialize the widget. */
  KMSearchRuleWidget( QWidget* parent=0, KMSearchRule* aRule=0, const char* name=0, bool headersOnly = false, bool absoluteDates = false );

  /** Set the rule. The rule is accepted regardless of the return
      value of @ref KMSearchRule::isEmpty. This widget makes a shallow
      copy of @p aRule and operates directly on it. If @p aRule is
      0, resets itself, taks user input, but does essentially
      nothing. If you pass 0, you should probably disable it. */
  void setRule( KMSearchRule* aRule );
  /** Return a reference to the currently worked-on @ref KMSearchRule. */
  KMSearchRule* rule() const;
  /** Resets the rule currently worked on and updates the widget
      accordingly. */
  void reset();

signals:
  /** This signal is emitted whenever the user alters the field.  The
     pseudo-headers <...> are returned in their i18n form, but stored
     in their english form in the rule. */
  void fieldChanged( const QString & );

  /** This signal is emitted whenever the user alters the
     contents/value of the rule. */
  void contentsChanged( const QString & );

protected:
  /** Used internally to translate i18n-ized pseudo-headers back to
      english */
  QString ruleFieldToEnglish(const QString & i18nVal) const;
  /** Used internally to find the corresponding index into the field
      ComboBox. Returns the index if found or -1 if the search failed, */
  int indexOfRuleField(const QString aName) const;

protected slots:
  void editRegExp();
  void functionChanged( int which );

private:
  void initWidget();
  void initLists(bool headersOnly, bool absoluteDates);

  QComboBox* mRuleField;
  QComboBox* mRuleFunc;
  QLineEdit* mRuleValue;
  QPushButton* mRuleEditBut;
  QDialog* mRegExpEditDialog;
  QStringList mFilterFieldList, mFilterFuncList;
};


class KMSearchRuleWidgetLister : public KWidgetLister
{
  Q_OBJECT

  friend class KMSearchPatternEdit;

public:
  KMSearchRuleWidgetLister( QWidget *parent=0, const char* name=0, bool headersOnly = false, bool absoluteDates = false );

  virtual ~KMSearchRuleWidgetLister();

  void setRuleList( QPtrList<KMSearchRule> * aList );

public slots:
  void reset();

protected:
  virtual void clearWidget( QWidget *aWidget );
  virtual QWidget* createWidget( QWidget *parent );

private:
  void regenerateRuleListFromWidgets();
  QPtrList<KMSearchRule> *mRuleList;
  bool mHeadersOnly;
  bool mAbsoluteDates;
};


/** This widget is intended to be used in the filter configuration as
    well as in the message search dialogs. It consists of a frame,
    inside which there are placed two radio buttons entitled "Match
    {all,any} of the following", followed by a vertical stack of @ref
    KMSearchRuleWidgets (initially two) and two buttons to add and
    remove, resp., additional @ref KMSearchWidget 's.

    To set the widget according to a given @ref KMSearchPattern, use
    @ref setSearchPattern; to initialize it (e.g. for a new, virgin
    rule), use @ref setSearchPattern with a 0 argument. The widget
    operates directly on a shallow(!) copy of the search rule. So
    while you actually don't really need @ref searchPattern, because
    you can always store a pointer to the current pattern yourself,
    you must not modify the currently worked-on pattern yourself while
    this widget holds a reference to it. The only exceptions are:

    @li If you edit a derived class, you can change aspects of the
    class that don't interfere with the @ref KMSearchPattern part. An
    example is @ref KMFilter, whose actions you can still edit while
    the @ref KMSearchPattern part of it is being acted upon by this
    widget.

    @li You can change the name of the pattern, but only using (this
    widget's) @ref setName. You cannot change the pattern's name
    directly, although this widget in itself doesn't let the user
    change it. This is because it auto-names the pattern to
    "<$field>:$contents" iff the pattern begins with "<".

    @short A widget which allows editing a set of KMSearchRule's.
    @author Marc Mutz <Marc@Mutz.com>
*/

class KMSearchPatternEdit : public QGroupBox  {
  Q_OBJECT
public:
  /** Constructor. The parent and name parameters are passed to the underlying
      @ref QGroupBox, as usual. */
  KMSearchPatternEdit(QWidget *parent=0, const char *name=0, bool headersOnly = false, bool absoluteDates = false);
  /** Constructor. This one allows you to set a title different from
      i18n("Search Criteria"). */
  KMSearchPatternEdit(const QString & title, QWidget *parent=0, const char *name=0, bool headersOnly = false, bool absoluteDates = false);
  ~KMSearchPatternEdit();

  /** Set the search pattern. Rules are inserted regardless of the
      return value of each rules' @ref KMSearchRule::isEmpty. This
      widget makes a shallow copy of @p aPattern and operates directly
      on it. */
  void setSearchPattern( KMSearchPattern* aPattern );

  /** Updates the search pattern according to the current widget values */
  void updateSearchPattern() { mRuleLister->regenerateRuleListFromWidgets(); }

public slots:
  /** Called when the widget should let go of the currently referenced
      filter and disable itself. */
  void reset();

signals:
    /** This signal is emitted whenever the name of the processed
        search pattern may have changed. */
  void maybeNameChanged();

private slots:
  void slotRadioClicked(int aIdx);
  void slotAutoNameHack();

private:
  void initLayout( bool headersOnly, bool absoluteDates );

  KMSearchPattern *mPattern;
  QRadioButton    *mAllRBtn, *mAnyRBtn;
  KMSearchRuleWidgetLister *mRuleLister;
};

#endif
