#ifndef EXPIRE_DLG
#define EXPIRE_DLG

#ifdef CHARSETS

#include <qdialog.h>

class QComboBox;
class QCheckBox;

class CharsetsDlg: public QDialog
{
    Q_OBJECT
public:
    CharsetsDlg(const char *message,const char *composer
                ,bool ascii,bool quote);
    ~CharsetsDlg();
public slots:
    void save();
signals:
    void setCharsets(const char *message,const char *composer
                     ,bool ascii,bool quote,bool def);
private:
    QComboBox *messageCharset;
    QComboBox *composerCharset;
    QCheckBox *setDefault;
    QCheckBox *quoteUnknown;
    QCheckBox *is7BitASCII;
};

#endif
#endif
