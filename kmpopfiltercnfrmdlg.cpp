/***************************************************************************
                          kmpopheadersdlg.cpp
                             -------------------
    begin                : Mon Oct 22 2001
    copyright            : (C) 2001 by Heiko Hund
                                       Thorsten Zachmann
    email                : heiko@ist.eigentlich.net
                           T.Zachmann@zagge.de
 ***************************************************************************/

#include <config.h>
#include "kmpopfiltercnfrmdlg.h"
#include "kmheaders.h"

#include <qlayout.h>
#include <qlabel.h>
#include <qheader.h>
#include <qcheckbox.h>
#include <qvgroupbox.h>
#include <qtimer.h>

#include <klocale.h>
#include <kio/global.h>

#include <assert.h>

////////////////////////////////////////
///  view
KMPopHeadersView::KMPopHeadersView(QWidget *aParent)
      : KListView(aParent)
{
  mColumnOf[Down] = addColumn(QIconSet(QPixmap(mDown)), QString::null, 24);
  mColumnOf[Later] = addColumn(QIconSet(QPixmap(mLater)), QString::null, 24);
  mColumnOf[Delete] = addColumn(QIconSet(QPixmap(mDel)), QString::null, 24);
  /*int subjCol =*/ addColumn(i18n("Subject"), 180);
  /*int sendCol =*/ addColumn(i18n("Sender"), 150);
  int dateCol = addColumn(i18n("Date"), 120);
  int sizeCol = addColumn(i18n("Size"), 80);

  setSelectionMode(QListView::NoSelection);
  setColumnAlignment(mColumnOf[Down], Qt::AlignHCenter);
  setColumnAlignment(mColumnOf[Later], Qt::AlignHCenter);
  setColumnAlignment(mColumnOf[Delete], Qt::AlignHCenter);
  setColumnAlignment(sizeCol, Qt::AlignRight);
  setSorting(dateCol, false);
  setShowSortIndicator(true);
  header()->setResizeEnabled(false, mColumnOf[Down]);
  header()->setResizeEnabled(false, mColumnOf[Later]);
  header()->setResizeEnabled(false, mColumnOf[Delete]);
  header()->setClickEnabled(false, mColumnOf[Down]);
  header()->setClickEnabled(false, mColumnOf[Later]);
  header()->setClickEnabled(false, mColumnOf[Delete]);
  header()->setMovingEnabled(false);

  mActionAt[mColumnOf[Down]] = Down;
  mActionAt[mColumnOf[Later]] = Later;
  mActionAt[mColumnOf[Delete]] = Delete;

  connect(this, SIGNAL(pressed(QListViewItem*, const QPoint&, int)),
        SLOT(slotPressed(QListViewItem*, const QPoint&, int)));
  connect(this->header(), SIGNAL(indexChange(int , int , int)),
        SLOT(slotIndexChanged(int , int , int)));
}

KMPopHeadersView::~KMPopHeadersView()
{
}

int KMPopHeadersView::mapToColumn(KMPopFilterAction aAction)
{
  return (mColumnOf.contains(aAction))? mColumnOf[aAction]: NoAction;
}

KMPopFilterAction KMPopHeadersView::mapToAction(int aColumn)
{
  return (mActionAt.contains(aColumn))? mActionAt[aColumn]: NoAction;
}

/** No descriptions */
void KMPopHeadersView::slotPressed(QListViewItem* aItem, const QPoint&,
        int aColumn) {
  if ( !aItem ) return;
  KMPopHeadersViewItem *item = dynamic_cast<KMPopHeadersViewItem*>(aItem);
  assert( item );
  item->check(mapToAction(aColumn));
}

void KMPopHeadersView::slotIndexChanged(int, int aFromIndex, int aToIndex)
{
  if(aFromIndex < aToIndex)
      --aToIndex;

  mActionAt[aToIndex] = mActionAt[aFromIndex];
  mActionAt.erase(aFromIndex);

  mColumnOf[mActionAt[aToIndex]] = aToIndex;
  mColumnOf.erase(mActionAt[aFromIndex]);
}

const char *KMPopHeadersView::mUnchecked[26] = {
"19 16 9 1",
"  c None",
"# c #000000",
". c #ffffff",
"a c #4a4c4a",
"b c #524c52",
"c c #efefef",
"e c #fff2ff",
"f c #f6f2f6",
"g c #fff6ff",
"                   ",
"                   ",
"         aaaa      ",
"       ba####aa    ",
"      a##....aac   ",
"      a#......ec   ",
"     a#........fc  ",
"     a#........gc  ",
"     a#........fc  ",
"     b#........gc  ",
"      a#......gc   ",
"      age....gec   ",
"       ccfgfgcc    ",
"         cccc      ",
"                   ",
"                   ",};

const char *KMPopHeadersView::mChecked[26] = {
"19 16 9 1",
"  c None",
"# c #000000",
". c #ffffff",
"a c #4a4c4a",
"b c #524c52",
"c c #efefef",
"e c #fff2ff",
"f c #f6f2f6",
"g c #fff6ff",
"                   ",
"                   ",
"         aaaa      ",
"       ba####aa    ",
"      a##....aac   ",
"      a#......ec   ",
"     a#...##...fc  ",
"     a#..####..gc  ",
"     a#..####..fc  ",
"     b#...##...gc  ",
"      a#......gc   ",
"      age....gec   ",
"       ccfgfgcc    ",
"         cccc      ",
"                   ",
"                   ",};

const char *KMPopHeadersView::mLater[25] = {
"16 16 8 1",
". c None",
"g c #303030",
"c c #585858",
"f c #a0a0a0",
"b c #c0c000",
"e c #dcdcdc",
"a c #ffff00",
"d c #ffffff",
"................",
"...........eaa..",
"..........eaaa..",
".........ebaab..",
".........eaaa...",
"........eaaab...",
"........eaaa....",
".......eaaab....",
"eaae..ebaccccc..",
"eaaae.eaacdedc..",
"ebaaabaaabcdc...",
".ebaaaaaa.fgf...",
"..ebaaaa..cec...",
"...ebaab.cdedc..",
"........eccccc..",
"................"};

const char *KMPopHeadersView::mDown[20] = {
"16 16 3 1",
". c None",
"a c #008000",
"b c #00c000",
"................",
"............aa..",
"...........aaa..",
"..........baab..",
"..........aaa...",
".........baab...",
".........aaa....",
"........aaab....",
".aa....baaa.....",
".aaa...aaa......",
".baaabaaab......",
"..baaaaaa.......",
"...baaaa........",
"....baab........",
"................",
"................"};

const char *KMPopHeadersView::mDel[19] = {
"16 16 2 1",
". c None",
"# c #c00000",
"................",
"................",
"..##.......##...",
"..###.....###...",
"...###...###....",
"....###.###.....",
".....#####......",
"......###.......",
".....#####......",
"....###.###.....",
"...###...###....",
"..###.....###...",
"..##.......##...",
"................",
"................",
"................"};


/////////////////////////////////////////
/////////////////////////////////////////
///  viewitem
/////////////////////////////////////////
/////////////////////////////////////////
KMPopHeadersViewItem::KMPopHeadersViewItem(KMPopHeadersView *aParent, KMPopFilterAction aAction)
      : KListViewItem(aParent)
{
  mParent = aParent;

  switch(aAction)
  {
  case Delete:
    check(Delete);
    break;
  case Down:
    check(Down);
    break;
  case Later:
  default:
    check(Later);
    break;
  }
}

KMPopHeadersViewItem::~KMPopHeadersViewItem()
{
}

void KMPopHeadersViewItem::check(KMPopFilterAction aAction)
{
  if(aAction != NoAction && !mChecked[aAction])
  {
    setPixmap(mParent->mapToColumn(Down), QPixmap(KMPopHeadersView::mUnchecked));
    setPixmap(mParent->mapToColumn(Later), QPixmap(KMPopHeadersView::mUnchecked));
    setPixmap(mParent->mapToColumn(Delete), QPixmap(KMPopHeadersView::mUnchecked));
    setPixmap(mParent->mapToColumn(aAction), QPixmap(KMPopHeadersView::mChecked));
    mChecked[Down] = false;
    mChecked[Later] = false;
    mChecked[Delete] = false;
    mChecked[aAction] = true;
  }
}

void KMPopHeadersViewItem::paintFocus(QPainter *, const QColorGroup &, const QRect &)
{
}

QString KMPopHeadersViewItem::key(int col, bool) const
{
  if (col == 3) return KMMsgBase::skipKeyword(text(col).lower());
  if (col == 5) return text(7);
  if (col == 6)
  {
    QString st = text(col);
    while (st.length() < 10) st = "0" + st;
    return st;
  }
  return text(col);
}

/////////////////////////////////////////
/////////////////////////////////////////
///  dlg
/////////////////////////////////////////
/////////////////////////////////////////
KMPopFilterCnfrmDlg::KMPopFilterCnfrmDlg(QPtrList<KMPopHeaders> *aHeaders, const QString &aAccount, bool aShowLaterMsgs, QWidget *aParent, const char *aName)
      : KDialogBase(aParent, aName, TRUE, i18n("KMail POP Filter"), Ok | Help, Ok, FALSE)
{
  unsigned int rulesetCount = 0;
  mHeaders = aHeaders;
  mShowLaterMsgs = aShowLaterMsgs;
  mLowerBoxVisible = false;

  QWidget *w = new QWidget(this);
  setMainWidget(w);

  QVBoxLayout *vbl = new QVBoxLayout(w, 0, spacingHint());

  QLabel *l = new QLabel(i18n("Messages to filter found on POP Account: <b>%1</b><p>"
      "The messages shown exceed the maximum size limit you defined for this account.<br>You can select "
      "what you want to do with them by checking the appropriate button.").arg(aAccount), w);
  vbl->addWidget(l);

  QVGroupBox *upperBox = new QVGroupBox(i18n("Messages Exceeding Size"), w);
  upperBox->hide();
  KMPopHeadersView *lv = new KMPopHeadersView(upperBox);
  vbl->addWidget(upperBox);

  QVGroupBox *lowerBox = new QVGroupBox(i18n("Ruleset Filtered Messages: none"), w);
  QString checkBoxText((aShowLaterMsgs)?
      i18n("Show messages matched by a ruleset and tagged 'Download' or 'Delete'"):
      i18n("Show messages matched by a filter ruleset"));
  QCheckBox* cb = new QCheckBox(checkBoxText, lowerBox);
  cb->setEnabled(false);
  mFilteredHeaders = new KMPopHeadersView(lowerBox);
  mFilteredHeaders->hide();
  vbl->addWidget(lowerBox);

  mFilteredHeaders->header()->setResizeEnabled(false, 7);
  mFilteredHeaders->setColumnWidth(7, 0);

  // fill the listviews with data from the headers
  KMPopHeaders *headers;
  for(headers = aHeaders->first(); headers; headers = aHeaders->next())
  {
    KMPopHeadersViewItem *lvi = 0;

    if(headers->ruleMatched())
    {
      if(aShowLaterMsgs && headers->action() == Later)
      {
        // insert messages tagged 'later' only
        lvi = new KMPopHeadersViewItem(mFilteredHeaders, headers->action());
        mFilteredHeaders->show();
        mLowerBoxVisible = true;
      }
      else if(aShowLaterMsgs)
      {
        // enable checkbox to show 'delete' and 'download' msgs
        // but don't insert them into the listview yet
        mDDLList.append(headers);
        cb->setEnabled(true);
      }
      else if(!aShowLaterMsgs)
      {
        // insert all messaged tagged by a ruleset, enable
        // the checkbox, but don't show the listview yet
        lvi = new KMPopHeadersViewItem(mFilteredHeaders, headers->action());
        cb->setEnabled(true);
      }
      rulesetCount++;
    }
    else
    {
      // insert all messages not tagged by a ruleset
      // into the upper listview
      lvi = new KMPopHeadersViewItem(lv, headers->action());
      upperBox->show();
    }

    if(lvi)
    {
      // todo: extrect the whole listitem thing into a method
      mItemMap[lvi] = headers;
      KMMessage *msg = headers->header();
      // set the subject
      QString tmp = msg->subject();
      if(tmp.isEmpty())
        tmp = i18n("no subject");
      lvi->setText(3, tmp);
      // set the sender
      tmp = msg->fromStrip();
      if(tmp.isEmpty())
        tmp = i18n("unknown");
      lvi->setText(4, msg->fromStrip());
      // set the date
      lvi->setText(5, KMime::DateFormatter::formatDate( KMime::DateFormatter::Fancy, msg->date() ) );
      // set the size
      lvi->setText(6, KIO::convertSize(headers->header()->msgLength()));
      // Date for sorting
      lvi->setText(7, msg->dateIsoStr());
    }
  }

  if(rulesetCount)
      lowerBox->setTitle(i18n("Ruleset Filtered Messages: %1").arg(rulesetCount));

  // connect signals and slots
  connect(lv, SIGNAL(pressed(QListViewItem*, const QPoint&, int)),
      this, SLOT(slotPressed(QListViewItem*, const QPoint&, int)));
  connect(mFilteredHeaders, SIGNAL(pressed(QListViewItem*, const QPoint&, int)),
      this, SLOT(slotPressed(QListViewItem*, const QPoint&, int)));
  connect(cb, SIGNAL(toggled(bool)),
      this, SLOT(slotToggled(bool)));

  adjustSize();
  QTimer::singleShot(0, this, SLOT(slotUpdateMinimumSize()));
}

KMPopFilterCnfrmDlg::~KMPopFilterCnfrmDlg()
{
}

/**
  This Slot is called whenever a ListView item was pressed.
  It checks for the column the button was pressed in and changes the action if the
  click happened over a radio button column.
  Of course the radio button state is changed as well if the above is true.
*/
void KMPopFilterCnfrmDlg::slotPressed(QListViewItem *aItem, const QPoint &, int aColumn)
{
  switch(aColumn)
  {
  case 0:
    mItemMap[aItem]->setAction(Down);
    break;
  case 1:
    mItemMap[aItem]->setAction(Later);
    break;
  case 2:
    mItemMap[aItem]->setAction(Delete);
  }
}

void KMPopFilterCnfrmDlg::slotToggled(bool aOn)
{
  if(aOn)
  {
    if(mShowLaterMsgs)
    {
      // show download and delete msgs in the list view too
      for(KMPopHeaders* headers = mDDLList.first(); headers; headers = mDDLList.next())
      {
      // todo: extract the whole listitem thing into a method
        KMPopHeadersViewItem *lvi = new KMPopHeadersViewItem(mFilteredHeaders, headers->action());
        mItemMap[lvi] = headers;
        mDelList.append(lvi);
        KMMessage *msg = headers->header();
        // set the subject
        QString tmp = msg->subject();
        if(tmp.isEmpty())
          tmp = i18n("no subject");
        lvi->setText(3, tmp);
        // set the sender
        tmp = msg->fromStrip();
        if(tmp.isEmpty())
          tmp = i18n("unknown");
        lvi->setText(4, msg->fromStrip());
        // set the date
        lvi->setText(5, KMime::DateFormatter::formatDate( KMime::DateFormatter::Fancy, msg->date() ) );
        // set the size
        lvi->setText(6, QString("%1").arg(headers->header()->msgLength()));
        // Date for sorting
        lvi->setText(7, msg->dateIsoStr());
      }
    }

    if(!mLowerBoxVisible)
    {
      mFilteredHeaders->show();
    }
  }
  else
  {
    if(mShowLaterMsgs)
    {
      // delete download and delete msgs from the lower listview
      for(KMPopHeadersViewItem* item = mDelList.first(); item; item = mDelList.next())
      {
        mFilteredHeaders->takeItem(item);
      }
      mDelList.clear();
    }

    if(!mLowerBoxVisible)
    {
      mFilteredHeaders->hide();
    }
  }
  QTimer::singleShot(0, this, SLOT(slotUpdateMinimumSize()));
}

void KMPopFilterCnfrmDlg::slotUpdateMinimumSize()
{
  mainWidget()->setMinimumSize(mainWidget()->sizeHint());
}

#include "kmpopfiltercnfrmdlg.moc"
