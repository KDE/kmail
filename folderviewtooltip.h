#ifndef __FOLDERVIEWTOOLTIP_H__
#define __FOLDERVIEWTOOLTIP_H__

#include <kmfoldercachedimap.h>

#include <qtooltip.h>

namespace KMail {

class FolderViewToolTip : public QObject
{
  public:
    FolderViewToolTip( Q3ListView* parent ) :
      QObject( parent->viewport() ),
      mListView( parent )
    {
      parent->viewport()->installEventFilter( this );
    }

    bool eventFilter( QObject *watched, QEvent *event )
    {
      if ( watched==mListView->viewport() && event->type()==QEvent::ToolTip ) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent*>( event );
        maybeTip( helpEvent->pos() );
        return true;
      } else {
        return QObject::eventFilter( watched, event );
      }
    }

  protected:
    void maybeTip( const QPoint &point )
    {
      KMFolderTreeItem *item = dynamic_cast<KMFolderTreeItem*>( mListView->itemAt( point ) );
      if  ( !item ) {
        QToolTip::hideText();
        return;
      }

      if ( !item->folder() || item->folder()->noContent() ) {
        QToolTip::hideText();
        return;
      }

      item->updateCount();
      QString tipText = i18n( "<qt><b>%1</b><br/>Total: %2<br/>Unread: %3<br/>Size: %4</qt>",
                              item->folder()->prettyUrl().replace( " ", "&nbsp;" ),
                              item->totalCount() < 0 ? "-"
                                : i18nc("Total number of emails.", "%1", item->totalCount()),
                              item->unreadCount() < 0 ? "-"
                                : i18nc("Number of unread emails.", "%1", item->unreadCount()),
                              KIO::convertSize( item->folderSize() ) );

      if ( KMFolderCachedImap* imap = dynamic_cast<KMFolderCachedImap*>( item->folder()->storage() ) ) {
          QuotaInfo info = imap->quotaInfo();
          if ( info.isValid() && !info.isEmpty() )
              tipText += i18n("<br/>Quota: %1", info.toString() );
      }

      QToolTip::showText( mListView->viewport()->mapToGlobal( point ), tipText, mListView );
    }

  private:
    Q3ListView *mListView;
};

}

#endif /* __FOLDERVIEWTOOLTIP_H__ */
