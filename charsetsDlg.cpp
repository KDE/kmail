#if defined CHARSETS

#include "charsetsDlg.h"

#include <qlayout.h>
#include <qlabel.h>
#include <qframe.h>
#include <qcheckbox.h>
#include <qpushbutton.h>

#include <kapp.h>
#include <klocale.h>
#include <kconfig.h>
#include <kcharsets.h>
#include <qcombobox.h>

#include <qlayout.h>

#include "charsetsDlg.moc"


extern KConfig *conf;

CharsetsDlg::CharsetsDlg(const char *message,const char *composer,
                         bool ascii,bool quote) :
#ifdef KRN
    QDialog (0,i18n("KRN - Charset Settings"),true)
#else
    QDialog (0,i18n("KMail - Charset Settings"),true)
#endif
{
int i;
    QBoxLayout *mainl=new QVBoxLayout(this);
    QBoxLayout *buttonsl=new QHBoxLayout();
    QBoxLayout *optl=new QHBoxLayout();
    KCharsets *charsets=kapp->getCharsets();
    
      
    QLabel *l=new QLabel(i18n("Message charset"),this,"l1");
    l->setAlignment(AlignCenter);
    mainl->addWidget(l);
    
    messageCharset=new QComboBox(this,"c1");
    mainl->addWidget(messageCharset);
    
    QStrList lst=charsets->registered();
    messageCharset->insertItem( "default" );
    for(const char *chset=lst.first();chset;chset=lst.next())
      messageCharset->insertItem( chset );
    int n=messageCharset->count();  
    for(i=0;i<n;i++)
      if (!stricmp(messageCharset->text(i),message)){
        messageCharset->setCurrentItem(i);
	break;
      }	
 
    l=new QLabel(i18n("Composer charset"),this,"l2");
    l->setAlignment(AlignCenter);
    mainl->addWidget(l);

    composerCharset=new QComboBox(this,"c2");
    mainl->addWidget(composerCharset);
    
    QStrList lst1=charsets->displayable();
    composerCharset->insertItem( "default" );
    for(const char *chset=lst1.first();chset;chset=lst1.next())
      composerCharset->insertItem( chset );
    n=composerCharset->count();  
    for(i=0;i<n;i++)
      if (!stricmp(composerCharset->text(i),composer)){
        composerCharset->setCurrentItem(i);
	break;
       }	
      
    setDefault=new QCheckBox(i18n("Set as &default"),this,"cb");
    mainl->addWidget(setDefault);
    
    mainl->addLayout(optl);
    
    is7BitASCII=new QCheckBox(i18n("&7 bit is ASCII"),this,"7b");
    is7BitASCII->setChecked(ascii);
    optl->addWidget(is7BitASCII);
    
    quoteUnknown=new QCheckBox(i18n("&Quote unknown characters"),this,"qu");
    quoteUnknown->setChecked(quote);
    optl->addWidget(quoteUnknown);

    mainl->addLayout(buttonsl);

    QPushButton *b1=new QPushButton(i18n("OK"),this,"b1");
    buttonsl->addWidget(b1);
    QPushButton *b2=new QPushButton(i18n("Cancel"),this,"b2");
    buttonsl->addWidget(b2);

    connect (b1,SIGNAL(clicked()),SLOT(save()));
    connect (b2,SIGNAL(clicked()),SLOT(reject()));

    mainl->activate();
}

void CharsetsDlg::save()
{
    emit setCharsets(messageCharset->currentText()
                     ,composerCharset->currentText()
		     ,is7BitASCII->isChecked()
		     ,quoteUnknown->isChecked()
		     ,setDefault->isChecked());
    accept();
}

CharsetsDlg::~CharsetsDlg()
{
}
#endif
