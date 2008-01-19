/*
 *  File : snippetwidget.h
 *
 *  Author: Robert Gruber <rgruber@users.sourceforge.net>
 *
 *  Copyright: See COPYING file that comes with this distribution
 */

#ifndef __SNIPPETWIDGET_H__
#define __SNIPPETWIDGET_H__

#include <qstring.h>
#include <QTreeWidget>
#include <qrect.h>

#include <libkdepim/kmeditor.h>
#include <ktexteditor/view.h>

class QPushButton;
class QTreeWidgetItem;
class QContextMenuEvent;
class SnippetDlg;
class SnippetItem;
class KTextEdit;
class KConfig;
class QWidget;
class KActionCollection;
using KPIM::KMeditor;

/**
@author Robert Gruber
*/
class SnippetWidget : public QTreeWidget
{
  Q_OBJECT

public:
    SnippetWidget(KMeditor *editor, KActionCollection *actionCollection, QWidget *parent = 0);
    ~SnippetWidget();
    QList<SnippetItem * > * getList() { return (&_list); }
    void writeConfig();

private slots:
    void readConfig();

protected:
    virtual void dragMoveEvent( QDragMoveEvent * event );
    virtual void dragEnterEvent( QDragEnterEvent * event );
    virtual void startDrag( Qt::DropActions supportedActions );
    virtual void dropEvent( QDropEvent * event );
    virtual void contextMenuEvent( QContextMenuEvent *e );

private:
    void insertIntoActiveView( const QString &text );
    QString parseText( const QString &text );
    QString showSingleVarDialog( const QString &var, QMap<QString, QString> * mapSave );
    QTreeWidgetItem *selectedItem() const;
    SnippetItem* makeItem( SnippetItem *parent, const QString &name,
                           const QString &text, const QKeySequence &keySeq );

    QList<SnippetItem * > _list;
    QMap<QString, QString> _mapSaved;
    KMeditor *mEditor;
    KActionCollection* mActionCollection;
    KConfig * _cfg;

public slots:
    void slotRemove();
    void slotEdit( QTreeWidgetItem *item_ = 0 );
    void slotEditGroup();
    void slotAdd();
    void slotAddGroup();
    void slotExecute();

protected slots:
    void slotExecuted( QTreeWidgetItem *item =  0 );
};

#endif
