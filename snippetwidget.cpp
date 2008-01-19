/*
 *  File : snippetwidget.cpp
 *
 *  Author: Robert Gruber <rgruber@users.sourceforge.net>
 *
 *  Copyright: See COPYING file that comes with this distribution
 */

#include "snippetwidget.h"
#include "snippetitem.h"
#include "snippetdlg.h"

#include <kdebug.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kmessagebox.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <kconfig.h>
#include <kmenu.h>
#include <kcombobox.h>
#include <kiconloader.h>
#include <kshortcut.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kkeysequencewidget.h>

#include <qlayout.h>
#include <qheaderview.h>
#include <qtooltip.h>
#include <qregexp.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qdrag.h>
#include <QContextMenuEvent>
#include <qtimer.h>

#include <cassert>

SnippetWidget::SnippetWidget(KMeditor* editor, KActionCollection* actionCollection, QWidget* parent)
 : QTreeWidget( parent ),
   mEditor( editor ),
   mActionCollection( actionCollection ),
   _cfg( 0 )
{
    setObjectName("snippet widget");
    // init the QTreeWidget
    //setSorting( -1 );
    //setFullWidth(true);
    header()->hide();
    setAcceptDrops(true);
    setDragEnabled(true);
    //setDropVisualizer(false);
    setRootIsDecorated(true);
    setAlternatingRowColors( true );

    connect( this, SIGNAL( itemActivated (QTreeWidgetItem *, int) ),
             this, SLOT( slotEdit( QTreeWidgetItem *) ) );

//### in KDE 3 the enter key used to insert the snippet in the editor
//    (slotExecute()) but the enterPressed signal is gone. It is also a problem
//    that pressing enter on an item and double-clicking is supposed have the
//    exact same effect.

    connect( editor, SIGNAL( insertSnippet() ), this, SLOT( slotExecute() ));

    QTimer::singleShot(0, this, SLOT(readConfig()));
}

SnippetWidget::~SnippetWidget()
{
  writeConfig();
  delete _cfg;

  /* We need to delete the child items before the parent items
     otherwise KMail would crash on exiting */
  int itemsLeft = _list.count();
  while (itemsLeft) {
    for (int i = 0; i < _list.count(); i++) {
      if (_list[i] && _list[i]->childCount() == 0) {
        delete _list[i];
        _list[i] = 0;
        itemsLeft--;
      }
    }
  }
}


/*!
    Opens the dialog to add a snippet
 */
void SnippetWidget::slotAdd()
{
  //kDebug(5006) << "Ender slotAdd()";
  SnippetDlg dlg( mActionCollection, this );
  dlg.setObjectName( "SnippetDlg" );

  /*check if the user clicked a SnippetGroup
    If not, we set the group variable to the SnippetGroup
    which the selected item is a child of*/
  SnippetGroup * group = dynamic_cast<SnippetGroup*>(selectedItem());
  if ( !group && selectedItem() )
    group = dynamic_cast<SnippetGroup*>(selectedItem()->parent());

  /* still no group, let's make a default one */
  if (!group ) {
    if ( _list.isEmpty() ) {
      group = new SnippetGroup(this, i18n("General"), SnippetGroup::getMaxId() );
      _list.append( group );
    } else {
      group = dynamic_cast<SnippetGroup*>( _list.first() );
    }
  }
  assert( group );

  /*fill the combobox with the names of all SnippetGroup entries*/
  foreach (SnippetItem *item, _list) {
    if (dynamic_cast<SnippetGroup*>(item)) {
      dlg.cbGroup->addItem(item->getName());
    }
  }
  dlg.cbGroup->setCurrentIndex( dlg.cbGroup->findText( group->getName() ) );

  if (dlg.exec() == QDialog::Accepted) {
      group = dynamic_cast<SnippetGroup*>(SnippetItem::findItemByName(dlg.cbGroup->currentText(), _list));
      _list.append( makeItem( group, dlg.snippetName->text(),
                              dlg.snippetText->toPlainText(),
                              dlg.keyWidget->keySequence() ) );
  }
}

SnippetItem* SnippetWidget::makeItem( SnippetItem *parent, const QString &name,
                                      const QString &text, const QKeySequence &keySeq )
{
  SnippetItem *item = new SnippetItem( parent, name, text );
  const QString actionName = i18n( "Snippet %1", name );
  const QString normalizedName = QString( actionName ).replace( " ", "_" );
  item->setToolTip( 0, text );
  if ( !mActionCollection->action( normalizedName ) ) {
    KAction *action = mActionCollection->addAction( normalizedName,
                                                        item, SLOT( slotExecute() ) );
    action->setText( actionName );
    action->setShortcut( keySeq );
    item->setAction( action );
    connect( item, SIGNAL( execute( QTreeWidgetItem* ) ),
             this, SLOT( slotExecuted( QTreeWidgetItem* ) ) );
  }
  return item;
}

/*!
    Opens the dialog to add a snippet
 */
void SnippetWidget::slotAddGroup()
{
  //kDebug(5006) << "Ender slotAddGroup()";
  SnippetDlg dlg( mActionCollection, this );
  dlg.setObjectName( "SnippetDlg" );
  dlg.setGroupMode( true );
  dlg.setWindowTitle( i18n("Add Group") );

  if (dlg.exec() == QDialog::Accepted) {
    _list.append( new SnippetGroup(this, dlg.snippetName->text(), SnippetGroup::getMaxId() ) );
  }
}


/*!
    Removes the selected snippet
 */
void SnippetWidget::slotRemove()
{
  //get current data
  QTreeWidgetItem * item = currentItem();
  SnippetItem *snip = dynamic_cast<SnippetItem*>( item );
  SnippetGroup *group = dynamic_cast<SnippetGroup*>( item );
  if (!snip)
    return;

  if (group) {
    if (group->childCount() > 0 &&
        KMessageBox::warningContinueCancel(this, i18n("Do you really want to remove this group and all its snippets?"),QString(), KStandardGuiItem::del())
        == KMessageBox::Cancel)
      return;

    for ( int i = 0; i < _list.size(); i++ ) {
      if (_list[i]->getParent() == group->getId()) {
        delete _list.takeAt( i-- );
      }
    }
  }

  //kDebug(5006) << "remove " << snip->getName();
  int snipIndex = _list.indexOf(snip);
  if (snipIndex > -1)
    delete _list.takeAt(snipIndex);
}



/*!
    Opens the dialog of editing the selected snippet
 */
void SnippetWidget::slotEdit( QTreeWidgetItem* item )
{
  if( item == 0 ) {
    item = currentItem();
  }

  SnippetGroup *pGroup = dynamic_cast<SnippetGroup*>(item);
  SnippetItem *pSnippet = dynamic_cast<SnippetItem*>( item );
  if (!pSnippet || pGroup) /*selected item must be a SnippetItem but MUST not be a SnippetGroup*/
    return;

  //init the dialog
  SnippetDlg dlg( mActionCollection, this );
  dlg.setObjectName( "SnippetDlg" );
  dlg.snippetName->setText(pSnippet->getName());
  dlg.snippetText->setText(pSnippet->getText());
  dlg.btnAdd->setText(i18n("&Apply"));
  if ( pSnippet->getAction() )
    dlg.keyWidget->setKeySequence( pSnippet->getAction()->shortcut().primary() );

  dlg.setWindowTitle(i18n("Edit Snippet"));
  /*fill the combobox with the names of all SnippetGroup entries*/
  foreach (SnippetItem *item, _list) {
    if (dynamic_cast<SnippetGroup*>(item)) {
      dlg.cbGroup->addItem( item->getName() );
    }
  }
  QString parentGroupText =
      SnippetItem::findGroupById( pSnippet->getParent(), _list )->getName();
  dlg.cbGroup->setCurrentIndex( dlg.cbGroup->findText( parentGroupText ) );

  if (dlg.exec() == QDialog::Accepted) {
    //update the QListView and the SnippetItem
    item->setText( 0, dlg.snippetName->text() );
    pSnippet->setName( dlg.snippetName->text() );
    pSnippet->setText( dlg.snippetText->toPlainText() );
    pSnippet->setToolTip( 0, pSnippet->getText() );
    if ( pSnippet->getAction() )
      pSnippet->getAction()->setShortcut( dlg.keyWidget->keySequence());

    /* if the user changed the parent we need to move the snippet */
    if ( SnippetItem::findGroupById(pSnippet->getParent(), _list)->getName() != dlg.cbGroup->currentText() ) {
      SnippetGroup * newGroup = dynamic_cast<SnippetGroup*>(SnippetItem::findItemByName(dlg.cbGroup->currentText(), _list));
      pSnippet->parent()->removeChild(pSnippet);
      newGroup->addChild(pSnippet);
      pSnippet->resetParent();
    }

    item->setSelected( true );
  }
}

/*!
    Opens the dialog of editing the selected snippet-group
 */
void SnippetWidget::slotEditGroup()
{
  //get current data
  QTreeWidgetItem * item = currentItem();

  SnippetGroup *pGroup = dynamic_cast<SnippetGroup*>( item );
  if (!pGroup) /*selected item MUST be a SnippetGroup*/
    return;

  //init the dialog
  SnippetDlg dlg( mActionCollection, this );
  dlg.setObjectName( "SnippetDlg" );
  dlg.setGroupMode( true );
  dlg.snippetName->setText(pGroup->getName());
  dlg.btnAdd->setText(i18n("&Apply"));
  dlg.setWindowTitle(i18n("Edit Group"));

  if (dlg.exec() == QDialog::Accepted) {
    //update the QListView and the SnippetGroup
    item->setText( 0, dlg.snippetName->text() );
    pGroup->setName( dlg.snippetName->text() );

    item->setSelected( true );
  }
}

void SnippetWidget::slotExecuted(QTreeWidgetItem * item)
{
  if( item == 0 ) {
    item = currentItem();
  }

  SnippetItem *pSnippet = dynamic_cast<SnippetItem*>( item );
  if (!pSnippet || dynamic_cast<SnippetGroup*>(item))
      return;

  //process variables if any, then insert into the active view
  insertIntoActiveView( parseText( pSnippet->getText() ) );
}


/*!
    Inserts the parameter text into the active view
 */
void SnippetWidget::insertIntoActiveView( const QString &text )
{
    mEditor->insertPlainText( text );
}


/*!
    Write the config file
 */
void SnippetWidget::writeConfig()
{
  if( !_cfg )
    return;
  _cfg->deleteGroup("SnippetPart");  //this is necessary otherwise delete entries will stay in list until
                                     //they get overwritten by a more recent entry
  KConfigGroup kcg = _cfg->group("SnippetPart");

  QString strKeyName="";
  QString strKeyText="";
  QString strKeyId="";

  int iSnipCount = 0;
  int iGroupCount = 0;

  foreach ( SnippetItem *const item, _list ) {  //write the snippet-list
    //kDebug(5006) << "SnippetWidget::writeConfig() " << item->getName();
    SnippetGroup * group = dynamic_cast<SnippetGroup*>(item);
    if (group) {
      //kDebug(5006) << "-->GROUP " << item->getName() << group->getId() << " " << iGroupCount;
      strKeyName=QString("snippetGroupName_%1").arg(iGroupCount);
      strKeyId=QString("snippetGroupId_%1").arg(iGroupCount);

      kcg.writeEntry(strKeyName, group->getName());
      kcg.writeEntry(strKeyId, group->getId());

      iGroupCount++;
    } else if (dynamic_cast<SnippetItem*>(item)) {
      //kDebug(5006) << "-->ITEM " << item->getName() << item->getParent() << " " << iSnipCount;
      strKeyName=QString("snippetName_%1").arg(iSnipCount);
      strKeyText=QString("snippetText_%1").arg(iSnipCount);
      strKeyId=QString("snippetParent_%1").arg(iSnipCount);

      kcg.writeEntry(strKeyName, item->getName());
      kcg.writeEntry(strKeyText, item->getText());
      kcg.writeEntry(strKeyId, item->getParent());

      KAction * action = item->getAction();
      if ( action ) {
        const KShortcut& sc = action->shortcut();
        if (!sc.isEmpty() ) {
          kcg.writeEntry( QString("snippetShortcut_%1").arg(iSnipCount), sc.toString() );
        }
      }
      iSnipCount++;
    } else {
      //kDebug(5006) << "-->ERROR " << item->getName();
    }
  }
  kcg.writeEntry("snippetCount", iSnipCount);
  kcg.writeEntry("snippetGroupCount", iGroupCount);

  int iCount = 1;
  QMap<QString, QString>::Iterator it;
  for ( it = _mapSaved.begin(); it != _mapSaved.end(); ++it ) {  //write the saved variable values
    if (it.value().length()<=0) continue;  //is the saved value has no length -> no need to save it

    strKeyName=QString("snippetSavedName_%1").arg(iCount);
    strKeyText=QString("snippetSavedVal_%1").arg(iCount);

    kcg.writeEntry(strKeyName, it.key());
    kcg.writeEntry(strKeyText, it.value());

    iCount++;
  }
  kcg.writeEntry("snippetSavedCount", iCount-1);
  kcg.sync();
}

/*!
    Initial read the config file
 */
void SnippetWidget::readConfig()
{
  if (!_cfg)
    _cfg = new KConfig("kmailsnippetrc", KConfig::NoGlobals);

  KConfigGroup kcg = _cfg->group("SnippetPart");

  QString strKeyName;
  QString strKeyText;
  QString strKeyId;
  SnippetItem *item;
  SnippetGroup *group;

  //kDebug(5006);

  //if entry doesn't get found, this will return -1 which we will need a bit later
  int iCount = kcg.readEntry("snippetGroupCount", -1);

  for ( int i=0; i<iCount; i++) {  //read the group-list
    strKeyName=QString("snippetGroupName_%1").arg(i);
    strKeyId=QString("snippetGroupId_%1").arg(i);

    QString strNameVal;
    int iIdVal=-1;

    strNameVal = kcg.readEntry(strKeyName, "");
    iIdVal = kcg.readEntry(strKeyId, -1);
    //kDebug(5006) << "Read group "  << " " << iIdVal;

    if ( !strNameVal.isEmpty() && iIdVal != -1 ) {
      group = new SnippetGroup(this, strNameVal, iIdVal);
      //kDebug(5006) << "Created group " << group->getName() << " " << group->getId();
      _list.append(group);
    }
  }

  /* Check if the snippetGroupCount property has been found
     if iCount is -1 this means, that the user has his snippets
     stored without groups. Therefore we will call the
     initConfigOldVersion() function below */
  // should not happen since this function has been removed

  if (iCount != -1) {
    iCount = kcg.readEntry("snippetCount", 0);
    for ( int i=0; i<iCount; i++) {  //read the snippet-list
        strKeyName=QString("snippetName_%1").arg(i);
        strKeyText=QString("snippetText_%1").arg(i);
        strKeyId=QString("snippetParent_%1").arg(i);

        QString strNameVal;
        QString strTextVal;
        int iParentVal = -1;

        strNameVal = kcg.readEntry(strKeyName, "");
        strTextVal = kcg.readEntry(strKeyText, "");
        iParentVal = kcg.readEntry(strKeyId, -1);
        //kDebug(5006) << "Read item " << strNameVal << " " << iParentVal;

        if ( !strNameVal.isEmpty() && !strTextVal.isEmpty() && iParentVal != -1) {
            KShortcut shortcut( kcg.readEntry( QString("snippetShortcut_%1").arg(i), QString() ) );
            item = makeItem( SnippetItem::findGroupById( iParentVal, _list ), strNameVal, strTextVal, shortcut.primary() );
            //kDebug(5006) << "Created item " << item->getName() << " " << item->getParent();
            _list.append(item);
        }
    }
  } else {
      //kDebug() << "iCount is not -1";
  }

  iCount = kcg.readEntry("snippetSavedCount", 0);

  for ( int i=1; i<=iCount; i++) {  //read the saved-values and store in QMap
    strKeyName=QString("snippetSavedName_%1").arg(i);
    strKeyText=QString("snippetSavedVal_%1").arg(i);

    QString strNameVal="";
    QString strTextVal="";

    strNameVal = kcg.readEntry(strKeyName, "");
    strTextVal = kcg.readEntry(strKeyText, "");

    if (strNameVal != "" && strTextVal != "") {
      _mapSaved[strNameVal] = strTextVal;
    }
  }
}


/*!
 * Shows the Popup-Menu if there is an item where the event occurred
*/
void SnippetWidget::contextMenuEvent( QContextMenuEvent *e )
{
    KMenu popup;

    SnippetItem *item = static_cast<SnippetItem *>(itemAt(e->pos()));
    if ( item ) {
        popup.addTitle( item->getName() );
        if (dynamic_cast<SnippetGroup*>(item)) {
            popup.addAction( i18n("Edit &group..."), this, SLOT( slotEditGroup() ) );
        } else {
            popup.addAction( KIcon("edit-paste"), i18n("&Paste"),
                             this, SLOT( slotExecuted() ) );
            popup.addAction( KIcon("document-properties"), i18n("&Edit..."),
                             this, SLOT( slotEdit() ) );
        }
        popup.addAction( KIcon("edit-delete"), i18n("&Remove"),
                         this, SLOT( slotRemove() ) );
        popup.addSeparator();
    } else {
        popup.addTitle(i18n("Text Snippets"));
    }
    popup.addAction( i18n("&Add Snippet..."), this, SLOT( slotAdd() ) );
    popup.addAction( i18n("Add G&roup..."), this, SLOT( slotAddGroup() ) );

    popup.exec(e->globalPos());
}

/*!
    This function is used to parse the given QString for variables. If found the user will be prompted
    for a replacement value. It returns the string text with all replacements made
 */
QString SnippetWidget::parseText( const QString &text )
{
  QString str = text;
  QString strName;
  QString strNew;
  QString strMsg;
  int iFound = -1;
  int iEnd = -1;
  QMap<QString, QString> mapVar;

  do {
    iFound = text.indexOf(QRegExp("\\$[A-Za-z-_0-9\\s]*\\$"), iEnd+1);  //find the next variable by this QRegExp
    if (iFound >= 0) {
      iEnd = text.indexOf('$', iFound+1)+1;
      strName = text.mid(iFound, iEnd-iFound);

      if ( strName != QString('$') + QString('$') ) {  //if not doubel-delimiter
        if ( mapVar[strName].length() <= 0 ) {  // and not already in map
          strMsg = i18n( "Please enter the value for <b>%1</b>:", strName );
          strNew = showSingleVarDialog( strName, &_mapSaved );
          if ( strNew.isEmpty() )
            return QString(); //user clicked Cancel
        } else {
          continue; //we have already handled this variable
        }
      }
      else {
        strNew = '$'; //if double-delimiter -> replace by single character
      }

      str.replace(strName, strNew);
      mapVar[strName] = strNew;
    }
  } while ( iFound != -1 );

  return str;
}

/*!
    This function constructs a dialog which contains a label and a linedit for the given variable
    It return either the entered value or an empty string if the user hit cancel
 */
QString SnippetWidget::showSingleVarDialog( const QString &var, QMap<QString, QString> *mapSave )
{
  // --BEGIN-- building a dynamic dialog
  QDialog dlg(this);
  dlg.setWindowTitle(i18n("Enter Values for Variables"));

  QGridLayout * layout = new QGridLayout( &dlg );
  QGridLayout * layoutTop = new QGridLayout();
  QGridLayout * layoutVar = new QGridLayout();
  QGridLayout * layoutBtn = new QGridLayout();
  layout->setMargin( 11 );
  layout->setSpacing( 6 );
  layoutTop->setMargin( 0 );
  layoutTop->setSpacing( 6 );
  layoutVar->setMargin( 0 );
  layoutVar->setSpacing( 6 );
  layoutBtn->setMargin( 0 );
  layoutBtn->setSpacing( 6 );
  layout->setObjectName( "layout" );
  layoutTop->setObjectName( "layoutTop" );
  layoutVar->setObjectName( "layoutVar" );
  layoutBtn->setObjectName( "layoutBtn" );

  KTextEdit *te = NULL;
  QLabel * labTop = NULL;
  QCheckBox * cb = NULL;

  labTop = new QLabel( &dlg );
  labTop->setObjectName( "label" );
  layoutTop->addWidget(labTop, 0, 0);
  labTop->setText( i18n("Enter the replacement values for %1:", var) );
  layout->addLayout( layoutTop, 0, 0, 1, 2 );


  cb = new QCheckBox( &dlg );
  cb->setObjectName( "cbVar" );
  cb->setChecked( false );
  cb->setText(i18n( "Make value &default" ));

  te = new KTextEdit( &dlg );
  te->setObjectName( "teVar" );
  layoutVar->addWidget( te, 0, 1, Qt::AlignTop);
  layoutVar->addWidget( cb, 1, 1, Qt::AlignTop);
  if ((*mapSave)[var].length() > 0) {
    cb->setChecked( true );
    te->setText((*mapSave)[var]);
  }

  cb->setToolTip( i18n("Enable this to save the value entered to the right as the default value for this variable") );
  cb->setWhatsThis( i18n("If you enable this option, the value entered to the right will be saved. "
                         "If you use the same variable later, even in another snippet, the value entered to the right "
                         "will be the default value for that variable.") );

  layout->addLayout( layoutVar, 1, 0, 1, 2 );

  KPushButton * btn1 = new KPushButton( KStandardGuiItem::cancel(), &dlg );
  btn1->setObjectName( "pushButton1") ;
  layoutBtn->addWidget( btn1, 0, 0 );

  KPushButton * btn2 = new KPushButton( KStandardGuiItem::apply(), &dlg );
  btn2->setObjectName( "pushButton2") ;
  btn2->setDefault( true );
  layoutBtn->addWidget( btn2, 0, 1 );

  layout->addLayout( layoutBtn, 2, 0, 1, 2 );
  te->setFocus();
  // --END-- building a dynamic dialog

  //connect the buttons to the QDialog default slots
  connect(btn1, SIGNAL(clicked()), &dlg, SLOT(reject()) );
  connect(btn2, SIGNAL(clicked()), &dlg, SLOT(accept()) );

  //execute the dialog
  QString strReturn = "";
  if ( dlg.exec() == QDialog::Accepted ) {
    if (cb->isChecked())     //if the checkbox is on; save the values for later
      (*mapSave)[var] = te->toPlainText();
    else
      (*mapSave).remove(var);

    strReturn = te->toPlainText();    //copy the entered values back the the given map
  }

  //do some cleanup
  delete cb;
  delete te;
  delete labTop;
  delete btn1;
  delete btn2;
  delete layoutTop;
  delete layoutVar;
  delete layoutBtn;
  delete layout;

  return strReturn;
}


/*!
    Emulate Qt3's selectedItem() because it does exactly what we need here.
*/
QTreeWidgetItem * SnippetWidget::selectedItem() const
{
  if ( selectedItems().isEmpty() )
    return 0;
  return currentItem();
}


/*!
    Check here if the data the user is about to drop fits our restrictions.
    We only accept dropps of plaintext, because from the dropped text
    we will create a snippet.
 */
void SnippetWidget::dragMoveEvent( QDragMoveEvent * event )
{
  QTreeWidgetItem * item = itemAt(event->pos());

  if (item &&
      event->mimeData()->hasText() &&
      static_cast<SnippetWidget *>(event->source()) != this) {
    event->setDropAction( Qt::CopyAction );
    event->accept();
  } else {
    event->ignore();
  }
}

void SnippetWidget::dragEnterEvent( QDragEnterEvent * event )
{
  if ( event->mimeData()->hasFormat("text/x-kmail-textsnippet") ||
       event->mimeData()->hasText() ) {
    event->setDropAction( Qt::CopyAction );
    event->accept();
  }
}

void SnippetWidget::dropEvent( QDropEvent * event )
{
  QTreeWidgetItem * item = itemAt( event->pos() );
  if ( !item || !event->mimeData()->hasText() ) {
    event->ignore();
    return;
  }
  else {
    event->setDropAction( Qt::CopyAction );
    event->accept();
  }

  SnippetItem *snippet = dynamic_cast<SnippetItem *>( item );
  SnippetGroup *group = dynamic_cast<SnippetGroup *>( item );
  SnippetGroup *parent = 0;
  if ( group )
    parent = group;
  else {
    assert( dynamic_cast<SnippetGroup*>( snippet->parent() ) );
    parent = static_cast<SnippetGroup*>( snippet->parent() );
  }

  // fill the dialog with the given data
  SnippetDlg dlg( mActionCollection, this );
  dlg.setObjectName( "SnippetDlg" );
  dlg.snippetName->clear();
  dlg.snippetText->setText( event->mimeData()->text() );

  /*fill the combobox with the names of all SnippetGroup entries*/
  foreach ( SnippetItem *const si, _list ) {
    if ( dynamic_cast<SnippetGroup*>( si ) ) {
      dlg.cbGroup->addItem( si->getName() );
    }
  }
  dlg.cbGroup->setCurrentIndex( dlg.cbGroup->findText( parent->getName() ) );

  if ( dlg.exec() == QDialog::Accepted ) {
    /* get the group that the user selected with the combobox */
    group = dynamic_cast<SnippetGroup*>(
             SnippetItem::findItemByName( dlg.cbGroup->currentText(), _list ) );
    _list.append( makeItem( group, dlg.snippetName->text(),
                  dlg.snippetText->toPlainText(), dlg.keyWidget->keySequence() ) );
  }
}

void SnippetWidget::startDrag( Qt::DropActions supportedActions )
{
  Q_UNUSED( supportedActions );

  // Don't allow to drag groups around
  if ( dynamic_cast<SnippetGroup*>( currentItem() ) )
    return;

  QString text = dynamic_cast<SnippetItem*>( currentItem() )->getText();
  QDrag *drag = new QDrag( this );
  QMimeData *mimeData = new QMimeData();
  mimeData->setData( "text/x-kmail-textsnippet", text.toUtf8() );
  drag->setMimeData( mimeData );
  drag->exec();
}

void SnippetWidget::slotExecute()
{
  slotExecuted(currentItem());
}

#include "snippetwidget.moc"
