#ifndef __FOLDERVIEWTOOLTIP_H__
#define __FOLDERVIEWTOOLTIP_H__

#include <kmfoldercachedimap.h>

#include <qtooltip.h>

namespace KMail {

class FolderViewToolTip : public QToolTip
{
  public:
    FolderViewToolTip( QListView* parent ) :
      QToolTip( parent->viewport() ),
      mListView( parent ) {}

  protected:
    void maybeTip( const QPoint &point )
    {
      KMFolderTreeItem *item = dynamic_cast<KMFolderTreeItem*>( mListView->itemAt( point ) );
      if  ( !item )
        return;
      const QRect itemRect = mListView->itemRect( item );
      if ( !itemRect.isValid() )
        return;
      const QRect headerRect = mListView->header()->sectionRect( 0 );
      if ( !headerRect.isValid() )
        return;
      
      if ( !item->folder() || item->folder()->noContent() )
        return;
      
      item->updateCount();
      QString tipText = i18n("<qt><b>%1</b><br>Total: %2<br>Unread: %3<br>Size: %4" )
          .arg( item->folder()->prettyURL().replace( " ", "&nbsp;" ) )
          .arg( item->totalCount() < 0 ? "-" : QString::number( item->totalCount() ) )
          .arg( item->unreadCount() < 0 ? "-" : QString::number( item->unreadCount() ) )
          .arg( KIO::convertSize( item->folderSize() ) );
      
      if ( KMFolderCachedImap* imap = dynamic_cast<KMFolderCachedImap*>( item->folder()->storage() ) ) {
          QuotaInfo info = imap->quotaInfo();
          if ( info.isValid() && !info.isEmpty() )
              tipText += i18n("<br>Quota: %1").arg( info.toString() );
      }
      
      tip( QRect( headerRect.left(), itemRect.top(), headerRect.width(), itemRect.height() ), tipText );
    }

  private:
    QListView *mListView;
};

}

#endif /* __FOLDERVIEWTOOLTIP_H__ */
