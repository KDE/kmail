/* Filter Dialog
 * Author: Marc Mutz <Marc@Mutz.com>,
 * based upon work by Stefan Taferner <taferner@kde.org>
 * This code is under GPL
 */
#ifndef kmfilterdlg_h
#define kmfilterdlg_h

#include "kwidgetlister.h"
#include "kmfilter.h"
#include "kmfilteraction.h"

#include <kdialogbase.h>

#include <qgroupbox.h>
#include <qhbox.h>
#include <qstring.h>
#include <qlist.h>

class KMSearchPatternEdit;
class QListBox;
class QPushButton;
class QComboBox;
class QWidgetStack;


/** This is a complex widget that is used to manipulate KMail's filter
    list. It consists of an internal list of filters, which is a deep
    copy of the list @ref KMFilterMgr manages, a @ref QListBox
    displaying that list, and a few buttons used to create new
    filters, delete them, rename them and change the order of filters.

    It does not provide means to change the actual filter (besides the
    name), but relies on auxiliary widgets (@ref KMSearchPatternEdit
    and @ref KMFilterActionEdit) to do that.

    Communication with this widget is quite easy: simply create an
    instance, connect the signals @ref filterSelected and
    @ref resetWidgets with a slot that does the right thing and there you
    go...

    This widget will operate on it's own copy of the filter list as
    long as you don't call @ref slotApplyFilterChanges. It will then
    transfer the altered filter list back to @ref KMFilterMgr.

    @short A complex widget that allows managing a list of KMFilter's.
    @author Marc Mutz <Marc@Mutz.com>, based upon work by Stefan Taferner <taferner@kde.org>.
    @see KMFilter KMFilterDlg KMFilterActionEdit KMSearchPatternEdit

 */
class KMFilterListBox : public QGroupBox
{
  Q_OBJECT
public:
  /* Constuctor. */
  KMFilterListBox( const QString & title, QWidget* parent=0, const char* name=0 );

  /** Called from @ref KMFilterDlg. Creates a new filter and presets
      the first rule with "field equals value". It's there mainly to
      support "rapid filter creation" from a context menu. You should
      instead call @ref KMFilterMgr::createFilter.
      @see KMFilterMgr::createFilter KMFilterDlg::createFilter
  */
  void createFilter( const QString field, const QString value );

  /** Loads the filter list and selects the first filter. Should be
      called when all signals are connected properly. */
  void loadFilterList();

signals:
  /** Emitted when a new filter has been selected by the user or if
      the current filter has changed after a 'new' or 'delete'
      operation. */
  void filterSelected( KMFilter* filter );

  /** Emitted when this widgets wants the edit widgets to let go of
      their filter reference. Everyone holding a reference to a filter
      should update it from the contents of the widgets used to edit
      it and set their internal reference to NULL. */
  void resetWidgets();

public slots:
  /** Called when the name of a filter might have changed (e.g. 
      through changing the first rule in @ref KMSearchPatternEdit).
      Updates the corresponding entry in the
      listbox and (if necessary) auto-names the filter. */
  void slotUpdateFilterName();
  /** Called when the user clicks either 'Apply' or 'OK' in @ref
      KMFilterDlg. Updates the filter list in the @ref KMFilterMgr. */
  void slotApplyFilterChanges();

protected slots:
  /** Called when the user clicks on a filter in the filter
      list. Calculates the corresponding filter and emits the
      @ref filterSelected signal. */
  void slotSelected( int aIdx );
  /** Called when the user clicks the 'New' button. Creates a new
      empty filter just before the current one. */
  void slotNew();
  /** Called when the user clicks the 'Delete' button. Deletes the
      current filter. */
  void slotDelete();
  /** Called when the user clicks the 'Up' button. Moves the current
      filter up one line. */
  void slotUp();
  /** Called when the user clicks the 'Down' button. Moves the current
      filter down one line. */
  void slotDown();
  /** Called when the user clicks the 'Rename' button. Pops up a
      dialog prompting to enter the new name. */
  void slotRename();

protected:
  /** The deep copy of the filter list. */
  QList<KMFilter> mFilterList;
  /** The listbox displaying the filter list. */
  QListBox *mListBox;
  /** The various action buttons. */
  QPushButton *mBtnNew, *mBtnDelete, *mBtnUp, *mBtnDown, *mBtnRename;
  /** The index of the currently selected item. */
  int mIdxSelItem;
private:
  void enableControls();
  void insertFilter( KMFilter* aFilter );
  void swapNeighbouringFilters( int untouchedOne, int movedOne);
};

/** This widgets allows to edit a single @ref KMFilterAction (in fact
    any derived class that is registered in
    @ref KMFilterActionDict). It consists of a combo box which allows to
    select the type of actions this widget should act upon and a
    @ref QWidgetStack, which holds the parameter widgets for the different
    rule types.

    You can load a @ref KMFilterAction into this widget with @ref
    setAction, and retrieve the result of user action with @ref action.
    The widget will copy it's setting into the corresponding
    parameter widget. For that, it internally creates an instance of
    every @ref KMFilterAction in @ref KMFilterActionDict and asks each
    one to create a parameter widget. The parameter widgets are put on
    the widget stack and are raised when their corresponding action
    type is selected in the combo box.

    @short A widget to edit a single KMFilterAction.
    @author Marc Mutz <Marc@Mutz.com>
    @see KMFilterAction KMFilter KMFilterActionWidgetLister

 */
class KMFilterActionWidget : public QHBox
{
  Q_OBJECT
public:
  /** Constructor. Creates a filter action widget with no type
      selected. */
  KMFilterActionWidget( QWidget* parent=0, const char* name=0 );

  /** Set an action. The action's type is determined and the
      corresponding widget it loaded with @p aAction's parameters and
      then raised. If @ aAction is NULL, the widget is cleared. */
  void setAction( const KMFilterAction * aAction );
  /** Retrieve the action. This method is necessary because the type
      of actions can change during editing. Therefore the widget
      always creates a new action object from the data in the combo
      box and the widget stack and returns that. */
  KMFilterAction *action();

private:
  /** This list holds an instance of every @ref KMFilterAction
      subclass. The only reason that these 'slave' actions exist is
      that they are 'forced' to create parameter widgets for the
      widget stack and to clear them on @ref setAction. */
  QList<KMFilterAction> mActionList;
  /** The combo box that contains the labels of all @ref KMFilterActions.
      It's @p activated(int) signal is internally
      connected to the @p raiseWidget(int) slot of @p mWidgetStack. */
  QComboBox      *mComboBox;
  /** The widget stack that holds all the parameter widgets for the
      filter actions. */
  QWidgetStack   *mWidgetStack;
};

class KMFilterActionWidgetLister : public KWidgetLister
{
  Q_OBJECT
public:
  KMFilterActionWidgetLister( QWidget *parent=0, const char* name=0 );

  virtual ~KMFilterActionWidgetLister();

  void setActionList( QList<KMFilterAction> * aList );

public slots:
  void reset();

protected:
  virtual void clearWidget( QWidget *aWidget );
  virtual QWidget* createWidget( QWidget *parent );

private:
  void regenerateActionListFromWidgets();
  QList<KMFilterAction> *mActionList;

};



/** The filter dialog. This is a non-modal dialog used to manage
    KMail's filters. It should only be called through @ref
    KMFilterMgr::openDialog. The dialog consists of three main parts:

    @li The @ref KMFilterListBox in the left half allows the user to
    select a filter to be displayed using the widgets on the right
    half. It also has buttons to delete filters, add new ones, to
    rename them and to change their order (maybe you will be able to
    move the filters around by dragging later, and to optimise the
    filters by trying to apply them to all locally available @ref
    KMMessage in turn and thus profiling which filters (and which
    rules of the search patterns) matches most often and sorting the
    filter/rules list according to the results, but I first want the
    basic functionality in place).

    @li The @ref KMSearchPatternEdit in the upper-right quarter allows
    the user to modify the filter criteria.

    @li The @ref KMFilterActionEdit in the lower-right quarter allows
    the user to select the actions that will be executed for any
    message that matches the search pattern.

    @li (tbi) There will be another widget that will allow the user to
    select to which folders the filter may be applied and whether it
    should be applied on outbound or inbound message transfers or both
    or none (meaning the filter is only applied when the user
    explicitely hits CTRL-J). I'm not sure whether there should be a
    per-folder filter list or a single list where you can select the
    names of folders this rule will be applied to.

    Upon creating the dialog, a (deep) copy of the current filter list
    is made by @ref KMFilterListBox. The changed filters are local to
    @ref KMFilterListBox until the user clicks the 'Apply' button.

    NOTE: Though this dialog is non-modal, it completely ignores all
    the stuff that goes on behind the scenes with folders esp. folder
    creation, move and create. The widgets that depend on the filter
    list and the filters that use folders as parameters are not
    updated as you expect. I hope this will change sometime soon.
    
    KMFilterDlg supports the creation of new filters through context
    menues, dubbed "rapid filters". Call @ref KMFilterMgr::createFilter
    to use this. That call will be delivered
    to this dialog, which in turn delivers it to the
    @ref KMFilterListBox.

    If you change the (DocBook) anchor for the filter dialog help,
    make sure to change @p const @p QString @p KMFilterDlgHelpAnchor
    in kmfilterdlg.cpp accordingly.

    @short The filter dialog.
    @author Marc Mutz <Marc@Mutz.com>, based upon work by Stefan Taferner <taferner@kde.org>.
    @see KMFilter KMFilterActionEdit KMSearchPatternEdit KMFilterListBox

 */

class KMFilterDlg: public KDialogBase
{
  Q_OBJECT
public:
  /** Create the filter dialog. The only class which should be able to
      do this is @ref KMFilterMgr. This ensures that there is only a
      single filter dialog */
  KMFilterDlg(QWidget* parent=0, const char* name=0);

  /** Called from @ref KMFilterMgr. Creates a new filter and presets
      the first rule with "field equals value". Internally forwarded
      to @ref KMFilterListBox::createFilter. You should instead call
      @ref KMFilterMgr::createFilter. */
  void createFilter( const QString field, const QString value )
    { mFilterList->createFilter( field, value ); }

public slots:
    /** Internally connected to @ref KMFilterListBox::filterSelected.
	Just does a simple check and then calls
	@ref KMSearchPatternEdit::setSearchPattern and
	@ref KMFilterActionEdit::setActionList. */
  void slotFilterSelected(KMFilter * aFilter);

protected:
  /** The widget that contains the ListBox showing the filters, and
      the controls to remove filters, add new ones and to change their
      order. */
  KMFilterListBox *mFilterList;
  /** The widget that allows editing of the filter pattern. */
  KMSearchPatternEdit *mPatternEdit;
  /** The widget that allows editing of the filter actions. */
  KMFilterActionWidgetLister *mActionLister;

};



#endif /*kmfilterdlg_h*/
