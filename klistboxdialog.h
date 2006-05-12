#ifndef KLISTBOXDIALOG_H
#define KLISTBOXDIALOG_H

#include <kdialog.h>

class QLabel;
class Q3ListBox;

class KListBoxDialog : public KDialog
{
    Q_OBJECT

public:
    KListBoxDialog( QString& _selectedString,
                    const QString& caption,
                    const QString& labelText,
                    QWidget*    parent = 0 );
    ~KListBoxDialog();
    
    void setLabelAbove(  const QString& label  );
    void setCommentBelow(const QString& comment);
    
    Q3ListBox* entriesLB;

private slots:
    void highlighted( const QString& );

protected:
    QString& selectedString;
    QLabel*  labelAboveLA;
    QLabel*  commentBelowLA;
};

#endif
