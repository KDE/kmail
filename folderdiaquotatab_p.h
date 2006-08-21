#ifndef FOLDERDIAQUOTA_P_H
#define FOLDERDIAQUOTA_P_H


#include <qlabel.h>
#include <qprogressbar.h>
#include <qwhatsthis.h>

#include "quotajobs.h"

namespace KMail {

class QuotaWidget : public QWidget {

 Q_OBJECT
public:
    QuotaWidget( QWidget* parent, const char* name = 0 );
    virtual ~QuotaWidget() { }
    void setQuotaInfo( const KMail::QuotaInfo& info );

private:
    void readConfig();

private:
    QLabel* mInfoLabel;
    QLabel* mRootLabel;
    QProgressBar* mProgressBar;
    QString mUnits;
    int mFactor;
};

}//end of namespace KMail

#endif /* FOLDERDIAQUOTA_H */
