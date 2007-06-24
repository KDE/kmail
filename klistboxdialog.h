#ifndef KLISTBOXDIALOG_H
#define KLISTBOXDIALOG_H

#include <kdialogbase.h>

class QLabel;
class QListBox;

class KListBoxDialog : public KDialogBase
{
    Q_OBJECT

public:
    KListBoxDialog( QString& _selectedString,
                    const QString& caption,
                    const QString& labelText,
                    QWidget*    parent = 0,
                    const char* name   = 0,
                    bool        modal  = true );
    ~KListBoxDialog();

    void setLabelAbove(  const QString& label  );
    void setCommentBelow(const QString& comment);

    QListBox* entriesLB;

private slots:
    void highlighted( const QString& );

protected:
    QString& selectedString;
    QLabel*  labelAboveLA;
    QLabel*  commentBelowLA;
};

#endif
