/*
  Filter Dialog
  Author: Marc Mutz <Marc@Mutz.com>
  based upon work by Stefan Taferner <taferner@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef kmfilterdlg_h
#define kmfilterdlg_h

#include "mailcommon/mailfilter.h"
#include "mailcommon/filteraction.h"

#include <kdialog.h>

#include <QGroupBox>
#include <QList>

class QLabel;
class QListWidget;
class QPushButton;
class QRadioButton;
class QCheckBox;
class QTreeWidget;
class KComboBox;
class KIconButton;
class KKeySequenceWidget;
class KActionCollection;

namespace MailCommon {
  class SearchPatternEdit;
  class FilterActionWidgetLister;
}


/** This is a complex widget that is used to manipulate KMail's filter
    list. It consists of an internal list of filters, which is a deep
    copy of the list KMFilterMgr manages, a QListBox displaying that list,
    and a few buttons used to create new filters, delete them, rename them
    and change the order of filters.

    It does not provide means to change the actual filter (besides the
    name), but relies on auxiliary widgets (SearchPatternEdit
    and KMFilterActionEdit) to do that.

    Communication with this widget is quite easy: simply create an
    instance, connect the signals filterSelected, resetWidgets
    and applyWidgets with a slot that does the right thing and there
    you go...

    This widget will operate on it's own copy of the filter list as
    long as you don't call slotApplyFilterChanges. It will then
    transfer the altered filter list back to KMFilterMgr.

    @short A complex widget that allows managing a list of MailCommon::MailFilter's.
    @author Marc Mutz <Marc@Mutz.com>, based upon work by Stefan Taferner <taferner@kde.org>.
    @see MailCommon::MailFilter KMFilterDlg KMFilterActionEdit SearchPatternEdit

 */
class KMFilterListBox : public QGroupBox
{
  Q_OBJECT
public:
  /** Constructor. */
  explicit KMFilterListBox( const QString & title, QWidget* parent=0);

  /** Destructor. */
  ~KMFilterListBox();

  /** Called from KMFilterDlg. Creates a new filter and presets
      the first rule with "field equals value". It's there mainly to
      support "rapid filter creation" from a context menu. You should
      instead call KMFilterMgr::createFilter.
      @see KMFilterMgr::createFilter KMFilterDlg::createFilter
  */
  void createFilter( const QByteArray & field, const QString & value );

  /** Loads the filter list and selects the first filter. Should be
      called when all signals are connected properly. If createDummyFilter
      is true, an empty filter is created to improve the usability of the
      dialog in case no filter has been defined so far.*/
  void loadFilterList( bool createDummyFilter );

  void insertFilter( MailCommon::MailFilter* aFilter );

  void appendFilter( MailCommon::MailFilter* aFilter );

  /** Returns a list of _copies_ of the current list of filters.
   * The list owns the contents and thus the caller needs to clean them up.
   * @param closeAfterSaving If true user is given option to continue editing
   * after being warned about invalid filters. Otherwise, user is just warned. */
  QList<MailCommon::MailFilter *> filtersForSaving( bool closeAfterSaving ) const;

signals:
  /** Emitted when a new filter has been selected by the user or if
      the current filter has changed after a 'new' or 'delete'
      operation. */
  void filterSelected( MailCommon::MailFilter* filter );

  /** Emitted when this widget wants the edit widgets to let go of
      their filter reference. Everyone holding a reference to a filter
      should update it from the contents of the widgets used to edit
      it and set their internal reference to 0. */
  void resetWidgets();

  /** Emitted when this widget wants the edit widgets to apply the changes
      to the current filter. */
  void applyWidgets();

  /** Emitted when the user decides to continue editing after being warned
   *  about invalid filters. */
  void abortClosing() const;

  /** Emitted when a new filter is created */
  void filterCreated();

  /** Emitted when a filter is deleted */
  void filterRemoved( MailCommon::MailFilter *filter );

  /** Emitted when a filter is updated (e.g. renamed) */
  void filterUpdated( MailCommon::MailFilter *filter );

  /** Emitted whenever the order in which the filters are displayed is changed*/
  void filterOrderAltered();
public slots:
  /** Called when the name of a filter might have changed (e.g.
      through changing the first rule in SearchPatternEdit).
      Updates the corresponding entry in the
      listbox and (if necessary) auto-names the filter. */
  void slotUpdateFilterName();
  /** Called when the user clicks either 'Apply' or 'OK' in
      KMFilterDlg. Updates the filter list in the KMFilterMgr. */
  void slotApplyFilterChanges( KDialog::ButtonCode );

protected slots:
  /** Called when the user clicks on a filter in the filter
      list. Calculates the corresponding filter and emits the
      filterSelected signal. */
  void slotSelected( int aIdx );
  /** Called when the user clicks the 'New' button. Creates a new
      empty filter just before the current one. */
  void slotNew();
  /** Called when the user clicks the 'Copy' button. Creates a copy
      of the current filter and inserts it just before the current one. */
  void slotCopy();
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
  QList<MailCommon::MailFilter *> mFilterList;
  /** The listbox displaying the filter list. */
  QListWidget *mListWidget;
  /** The various action buttons. */
  QPushButton *mBtnNew, *mBtnCopy, *mBtnDelete, *mBtnUp, *mBtnDown, *mBtnRename;
  /** The index of the currently selected item. */
  int mIdxSelItem;
  bool mShowLater;
private:
  void enableControls();

  void swapNeighbouringFilters( int untouchedOne, int movedOne);
};

/** The filter dialog. This is a non-modal dialog used to manage
    KMail's filters. It should only be called through KMFilterMgr::openDialog.
    The dialog consists of three main parts:

    @li The KMFilterListBox in the left half allows the user to
    select a filter to be displayed using the widgets on the right
    half. It also has buttons to delete filters, add new ones, to
    rename them and to change their order (maybe you will be able to
    move the filters around by dragging later, and to optimize the
    filters by trying to apply them to all locally available
    KMMessage in turn and thus profiling which filters (and which
    rules of the search patterns) matches most often and sorting the
    filter/rules list according to the results, but I first want the
    basic functionality in place).

    @li The SearchPatternEdit in the upper-right quarter allows
    the user to modify the filter criteria.

    @li The KMFilterActionEdit in the lower-right quarter allows
    the user to select the actions that will be executed for any
    message that matches the search pattern.

    @li (tbi) There will be another widget that will allow the user to
    select to which folders the filter may be applied and whether it
    should be applied on outbound or inbound message transfers or both
    or none (meaning the filter is only applied when the user
    explicitly hits CTRL-J). I'm not sure whether there should be a
    per-folder filter list or a single list where you can select the
    names of folders this rule will be applied to.

    Upon creating the dialog, a (deep) copy of the current filter list
    is made by KMFilterListBox. The changed filters are local to
    KMFilterListBox until the user clicks the 'Apply' button.

    NOTE: Though this dialog is non-modal, it completely ignores all
    the stuff that goes on behind the scenes with folders esp. folder
    creation, move and create. The widgets that depend on the filter
    list and the filters that use folders as parameters are not
    updated as you expect. I hope this will change sometime soon.

    KMFilterDlg supports the creation of new filters through context
    menus, dubbed "rapid filters". Call KMFilterMgr::createFilter
    to use this. That call will be delivered to this dialog, which in
    turn delivers it to the KMFilterListBox.

    If you change the (DocBook) anchor for the filter dialog help,
    make sure to change @p const @p QString @p KMFilterDlgHelpAnchor
    in kmfilterdlg.cpp accordingly.

    @short The filter dialog.
    @author Marc Mutz <Marc@Mutz.com>, based upon work by Stefan Taferner <taferner@kde.org>.
    @see MailCommon::MailFilter KMFilterActionEdit SearchPatternEdit KMFilterListBox

 */

class KMFilterDlg: public KDialog
{
  Q_OBJECT
public:
  /** Create the filter dialog. The only class which should be able to
      do this is KMFilterMgr. This ensures that there is only a
      single filter dialog */
  explicit KMFilterDlg( const QList<KActionCollection*>& actionCollection, QWidget* parent=0, bool createDummyFilter=true );

  /** Called from KMFilterMgr. Creates a new filter and presets
      the first rule with "field equals value". Internally forwarded
      to KMFilterListBox::createFilter. You should instead call
      KMFilterMgr::createFilter. */
  void createFilter( const QByteArray & field, const QString & value )
    { mFilterList->createFilter( field, value ); }

public slots:
  /**
   * Internally connected to KMFilterListBox::filterSelected.
   * Just does a simple check and then calls
   * SearchPatternEdit::setSearchPattern and
   * KMFilterActionEdit::setActionList.
   */
  void slotFilterSelected(MailCommon::MailFilter * aFilter);
  /** Override QDialog::accept to allow disabling close */
  virtual void accept();

protected slots:
  void slotApplicabilityChanged();
  void slotApplicableAccountsChanged();
  void slotStopProcessingButtonToggled( bool aChecked );
  void slotConfigureShortcutButtonToggled( bool aChecked );
  void slotShortcutChanged( const QKeySequence &newSeq );
  void slotConfigureToolbarButtonToggled( bool aChecked );
  void slotFilterActionIconChanged( const QString &icon );
  void slotReset();
  void slotUpdateFilter();
  void slotSaveSize();
  // called when the dialog is closed (finished)
  void slotFinished();
  // update the list of accounts shown in the advanced tab
  void slotUpdateAccountList();


  /** Called when a user clicks the import filters button. Pops up
   * a dialog asking the user which file to import from and which
   * of the filters in that file to import. */
  void slotImportFilters();

  /** Called when a user clicks the export filters button. Pops up
   * a dialog asking the user which filters to export and which
   * file to export to. */
  void slotExportFilters();

  /** Called when a user decides to continue editing invalid filters */
  void slotDisableAccept();

  /** Called whenever a change in the filters configuration is detected,
   *  to enable the Apply button
   */
  void slotDialogUpdated();

  /** Called wherenever the apply button is pressed */
  void slotApply();
protected:
  /** The widget that contains the ListBox showing the filters, and
      the controls to remove filters, add new ones and to change their
      order. */
  KMFilterListBox *mFilterList;
  /** The widget that allows editing of the filter pattern. */
  MailCommon::SearchPatternEdit *mPatternEdit;
  /** The widget that allows editing of the filter actions. */
  MailCommon::FilterActionWidgetLister *mActionLister;
  /** Lets the user select whether to apply this filter on
      inbound/outbound messages, both, or only on explicit CTRL-J. */
  QCheckBox *mApplyOnIn, *mApplyOnOut, *mApplyBeforeOut, *mApplyOnCtrlJ;
  /** For a filter applied to inbound messages selects whether to apply
      this filter to all accounts or to selected accounts only. */
  QRadioButton *mApplyOnForAll, *mApplyOnForTraditional, *mApplyOnForChecked;
  /** ListView that shows the accounts in the advanced tab */
  QTreeWidget *mAccountList;

  QCheckBox *mStopProcessingHere;
  QCheckBox *mConfigureShortcut;
  QCheckBox *mConfigureToolbar;
  QLabel *mFilterActionLabel;
  KIconButton *mFilterActionIconButton;
  KKeySequenceWidget *mKeySeqWidget;
  QGroupBox *mAdvOptsGroup;
  QGroupBox *mGlobalsBox;
  QCheckBox *mShowLaterBtn;

  MailCommon::MailFilter *mFilter;
  bool mDoNotClose;
  bool mIgnoreFilterUpdates;
};



#endif /*kmfilterdlg_h*/
