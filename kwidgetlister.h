// A widget which shows rows of other widgets and
// allows to add or remove widgets at/from the end.
// Author: Marc Mutz <Marc@Mutz.com>
// This code is under the GPL

#ifndef _KWIDGETLISTER_H_
#define _KWIDGETLISTER_H_

#include <qwidget.h>
#include <qlist.h>

class QPushButton;
class QVBoxLayout;
class QHBox;

/** Simple widget that nonetheless does a lot of the dirty work for
    the filter edit widgets (@ref KMSearchpatternEdit and @ref
    KMFilterActionEdit). It provides a growable and shrinkable area
    where widget may be diplayed in rows. Widgets can be added by
    hitting the provided 'More' button, removed by the 'Fewer' button
    and cleared (e.g. reset, if an derived class implements that and
    removed for all but @ref mMinWidgets).

    To use this widget, derive from it with the template changed to
    the type of widgets this class should list. Then reimplement @ref
    addWidgetAtEnd, @ref removeLastWidget, calling the original
    implementation as necessary. Instantate an object of the class and
    put it in your dialog.

    @short Widget that manages a list of other widgets (incl. 'more', 'fewer' and 'clear' buttons).
    @author Marc Mutz <Marc@Mutz.com>
    @see KMSearchPatternEdit::WidgetLister KMFilterActionEdit::WidgetLister

*/

class KWidgetLister : public QWidget
{
  Q_OBJECT
public:
  KWidgetLister( int minWidgets=1, int maxWidgets=8, QWidget* parent=0, const char* name=0 );
  virtual ~KWidgetLister();

protected slots:
  /** Called whenever the user clicks on the 'more' button.
      Reimplementations should call this method, because this
      implementation does all the dirty work with adding the widgets
      to the layout (through @ref addWidgetAtEnd) and enabling/disabling
      the control buttons. */
  virtual void slotMore();
  /** Called whenever the user clicks on the 'fewer' button.
      Reimplementations should call this method, because this
      implementation does all the dirty work with removing the widgets
      from the layout (through @ref removelastWidget) and
      enabling/disabling the control buttons. */
  virtual void slotFewer();
  /** Called whenever the user clicks on the 'clear' button.
      Reimplementations should call this method, because this
      implementation does all the dirty work with removing all but
      @ref mMinWidets widgets from the layout and enabling/disabling
      the control buttons. */
  virtual void slotClear();



protected:
  /** Adds a single widget. Doesn't care if there are already @ref
      mMaxWidgets on screen and whether it should enable/disable any
      controls. It simply does what it is asked to do.  You want to
      reimplement this method if you want to initialize the the widget
      when showing it on screen. Make sure you call this
      implementaion, though, since you cannot put the widget on screen
      from derived classes (@p mLayout is private). */
  virtual void addWidgetAtEnd();
  /** Removes a single (always the last) widget. Doesn't care if there
      are still only @ref mMinWidgets left on screen and whether it
      should enable/disable any controls. It simply does what it is
      asked to do. You want to reimplement this method if you want to
      save the the widget's state before removing it from screen. Make
      sure you call this implementaion, though, since you should not
      remove the widget from screen from derived classes. */
  virtual void removeLastWidget();
  /** Called to clear a given widget. The default implementation does
      nothing. */
  virtual void clearWidget( QWidget* );
  /** Because QT 2.x does not support signals/slots in template
      classes, we are forced to emulate this by forcing the
      implementers of subclasses of KWidgetLister to reimplement this
      function which replaces the "@p new @p T" call. */
  virtual QWidget* createWidget( QWidget *parent );
  /** Sets the number of widgets on scrren to exactly @p aNum. Doesn't
      check if @p aNum is inside the range @p
      [mMinWidgets,mMaxWidgets]. */
  virtual void setNumberOfShownWidgetsTo( int aNum );
  /** The list of widgets. Note that this list is set to auto-delete,
      meaning that widgets that are removed from the screen by either
      @ref slotFewer or @ref slotClear will be destroyed! */
  QList<QWidget> mWidgetList;
  /** The minimum number of widgets that are to stay on screen. */
  int mMinWidgets;
  /** The maximum number of widgets that are to be shown on screen. */
  int mMaxWidgets;

private:
  void enableControls();

  QPushButton *mBtnMore, *mBtnFewer, *mBtnClear;
  QVBoxLayout *mLayout;
  QHBox       *mButtonBox;
};



#endif /* _KWIDGETLISTER_H_ */
