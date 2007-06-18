#ifndef __KMAIL__MANAGESIEVESCRIPTSDIALOG_P_H__
#define __KMAIL__MANAGESIEVESCRIPTSDIALOG_P_H__

#include <kdialog.h>

#include <QTextEdit>

namespace KMail {

class SieveEditor : public KDialog {
  Q_OBJECT
  Q_PROPERTY( QString script READ script WRITE setScript )
public:
  explicit SieveEditor( QWidget * parent=0, const char * name=0 );
  ~SieveEditor();

  QString script() const { return mTextEdit->text(); }
  void setScript( const QString & script ) { mTextEdit->setText( script ); }

private:
  QTextEdit * mTextEdit;
};

}

#endif /* __KMAIL__MANAGESIEVESCRIPTSDIALOG_P_H__ */

