// $Id$

#include "knewpanner.h"

KNewPanner::KNewPanner(QWidget *parent= 0, const char *name= 0,
			   Orientation orient= Vertical, Units units= Percent, int pos= 50)
    : QWidget(parent, name)
{
    orientation= orient;
    currentunits= units;
    position= pos;
    initialised= false;
}

KNewPanner::~KNewPanner()
{
}

void KNewPanner::activate(QWidget *c0, QWidget *c1)
{
    int minx, maxx, miny, maxy, pos;

    child0= c0;
    child1= c1;

    // Set the minimum and maximum sizes
    if (orientation == Horizontal) {
	miny= c0->minimumSize().height() + c1->minimumSize().height()+4;
	maxy= c0->maximumSize().height() + c1->maximumSize().height()+4;
	minx= (c0->minimumSize().width() > c1->minimumSize().width())
	    ? c0->minimumSize().width() : c1->minimumSize().width();
	maxx= (c0->maximumSize().width() > c1->maximumSize().width())
	    ? c0->maximumSize().width() : c1->maximumSize().width();

	miny= (miny > 4) ? miny : 4;
	maxy= (maxy < 2000) ? maxy : 2000;
	minx= (minx > 2) ? minx : 2;
	maxx= (maxx < 2000) ? maxx : 2000;

	setMinimumSize(minx, miny);
	setMaximumSize(maxx, maxy);
    }
    else {
	minx= c0->minimumSize().width() + c1->minimumSize().width()+4;
	maxx= c0->maximumSize().width() + c1->maximumSize().width()+4;
	miny= (c0->minimumSize().height() > c1->minimumSize().height())
	    ? c0->minimumSize().height() : c1->minimumSize().height();
	maxy= (c0->maximumSize().height() > c1->maximumSize().height())
	    ? c0->maximumSize().height() : c1->maximumSize().height();

	minx= (minx > 4) ? minx : 4;
	maxx= (maxx < 2000) ? maxx : 2000;
	miny= (miny > 2) ? miny : 2;
	maxy= (maxy < 2000) ? maxy : 2000;

	setMinimumSize(minx, miny);
	setMaximumSize(maxx, maxy);
    }

    divider= new QFrame(this, "pannerdivider");
    divider->setFrameStyle(QFrame::Panel | QFrame::Raised);
    divider->setLineWidth(1);

    if (orientation == Horizontal)
        divider->setCursor(QCursor(sizeVerCursor));
    else
        divider->setCursor(QCursor(sizeHorCursor));

    divider->installEventFilter(this);

    initialised= true;

    pos= position;
    position=0;

    setSeperatorPos(pos);
}

int KNewPanner::seperatorPos()
{
    if (currentunits == Percent) return (position * 100 / width());
    return position;
}

void KNewPanner::setSeperatorPos(int pos)
{
    if (currentunits == Percent)
	setAbsSeperatorPos(pos * width() / 100);
    else
	setAbsSeperatorPos(pos);
}

void KNewPanner::setAbsSeperatorPos(int pos)
{
    pos= checkValue(pos);

    if (pos != position)
    {
	position= pos;
	resizeEvent(0);
    }
}

int KNewPanner::absSeperatorPos()
{
    return position;
}

KNewPanner::Units KNewPanner::units()
{
    return currentunits;
}

void KNewPanner::setUnits(Units u)
{
    currentunits= u;
}

void KNewPanner::resizeEvent(QResizeEvent* e)
{
    int w, oldW;

    if (e)
    {
        if (orientation == Horizontal)
	{
	    w = e->size().height();
	    oldW = e->oldSize().height();
	}
	else
	{
	    w = e->size().width();
	    oldW = e->oldSize().width();
	}
        position = (position * w / oldW);
    }

    if (orientation == Horizontal)
    {
        child0->setGeometry(0, 0, width(), position);
        child1->setGeometry(0, position+4, width(), height()-position-4);
	divider->setGeometry(0, position, width(), 4);
    }
    else
    {
        child0->setGeometry(0, 0, position, height());
        child1->setGeometry(position+4, 0, width()-position-4, height());
	divider->setGeometry(position, 0, 4, height());
    }
}

int KNewPanner::checkValue(int pos)
{
    if (orientation == Vertical)
    {
	if (pos < (child0->minimumSize().width()))
	    pos= child0->minimumSize().width();
	if ((width()-4-pos) < (child1->minimumSize().width()))
	    pos= width() - (child1->minimumSize().width()) -4;
    }
    else
    {
	if (pos < (child0->minimumSize().height()))
	    pos= (child0->minimumSize().height());
	if ((height()-4-pos) < (child1->minimumSize().height()))
	    pos= height() - (child1->minimumSize().height()) -4;
    }

    if (pos < 0) pos= 0;
    if (pos > width()) pos= width();

    return pos;
}

bool KNewPanner::eventFilter(QObject *, QEvent *e)
{
    QMouseEvent *mev;
    bool handled= false;

    switch (e->type())
    {
    case Event_MouseMove:
        mev= (QMouseEvent *)e;
	child0->setUpdatesEnabled(false);
	child1->setUpdatesEnabled(false);
	if (orientation == Horizontal)
	{
	    position= checkValue(position+mev->y());
	    divider->setGeometry(0, position, width(), 4);
	    divider->repaint(0);
	}
	else
	{
	    position= checkValue(position+mev->x());
	    divider->setGeometry(position, 0, 4, height());
	    divider->repaint(0);
	}
	handled= true;
	break;

    case Event_MouseButtonRelease:
	mev= (QMouseEvent *)e;

	child0->setUpdatesEnabled(true);
	child1->setUpdatesEnabled(true);

	if (orientation == Horizontal)
	{
	    setAbsSeperatorPos(absSeperatorPos());
	    resizeEvent(0);
	    child0->repaint(true);
	    child1->repaint(true);
	    divider->repaint(true);
	}
	else
	{
	    setAbsSeperatorPos(absSeperatorPos());
	    resizeEvent(0);
	    child0->repaint(true);
	    child1->repaint(true);
	    divider->repaint(true);
	}
	handled= true;
	break;
    }

    return handled;
}

#include "knewpanner.moc"



