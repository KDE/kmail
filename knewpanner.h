// -*- c++ -*-

#ifndef KNEWPANNER_H
#define KNEWPANNER_H

#include <qwidget.h>
#include <qframe.h>

/**
 * KNewPanner is a simple widget for managing two children which
 * are seperated by a draggable divider. The user can resize both
 * children by moving the bar. This widget fixes a number of design
 * problems in the original KPanner class which show up particularly
 * if you try to use it under a layout manager (such as QBoxLayout).
 *
 * PLEASE NOTE: This panner is NOT source or binary compatable with
 * the old one.
 *
 * @version $Id$
 * @author Richard Moore rich@kde.org
 */
class KNewPanner : public QWidget
{
    Q_OBJECT

public:
    /**
     * Constants used to specify the orientation.
     */
    enum Orientation { Vertical, Horizontal };

    /**
     * Constants used to choose between absolute (pixel) sizes and
     * percentages.
     */
    enum Units { Percent, Absolute };

    /**
     * Construct a KNewPanner widget.
     *
     * @param parent  The parent widget
     * @param name  The name of the panner widget.
     * @param orient  The orientation of the panner, one of the constants
     *   Vertical or Horizontal.
     * @param units  The units in which you want to specify the seperator position.
     * @param pos  The initial seperator position in the selected units.
     */
    KNewPanner(QWidget *parent= 0, const char *name= 0,
		 Orientation orient= Vertical, Units units= Percent, int pos= 50);

    /**
     * Clean up
     */
    virtual ~KNewPanner();

    /**
     * Begin managing these two widgets. If you want to set the minimum or
     * maximum sizes of the children then you should do it before calling this
     * method.
     */
    void activate(QWidget *c0, QWidget *c1);

    /**
     * This gets the current position of the seperator in the current units.
     */
    int seperatorPos();

    /**
     * This sets the position of the seperator to the specified position. The position
     * is specified in the currently selected units.
     */
    void setSeperatorPos(int pos);

    /**
     * This gets the current position of the seperator in absolute 
     * (pixel) units.
     */
    int absSeperatorPos();

    /**
     * This sets the position of the seperator to the specified position.
     * The position is specified in absolute units (pixels) irrespective
     * of the the currently selected units.
     */
    void setAbsSeperatorPos(int pos);

    /**
     * Get the current units.
     */
    Units units();

    /**
     * Set the current units.
     */
    void setUnits(Units);

protected:
    /**
     * This returns the closest valid absolute seperator position to the
     * position specified in the parameter.
     */
    int checkValue(int);

    /**
     * This method handles changes in the panner size, it will automatically resize
     * the child widgets as appropriate.
     */
    void resizeEvent(QResizeEvent *);

private:
    // The managed children
    QWidget *child0, *child1;
    bool initialised;

    // The divider widget
    QFrame *divider;

    bool eventFilter(QObject *, QEvent *);

    // The position in pixel units
    int position;
    Units currentunits;
    Orientation orientation;
};

#endif





