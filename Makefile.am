SUBDIRS = pics doc
INCLUDES = $(all_includes)
LDADD	= $(all_libraries) -lkhtmlw -ljscript -lkfile -lkfm -lkdeui -lkdecore -lqt -lXext -lX11 $(LIBJPEG) -lmimelib -lstdc++ $(LIBSOCKET)
#DEFS += -DCHARSETS

bin_PROGRAMS = kmail

kmail_LDFLAGS = $(LDFLAGS) $(KDE_RPATH)
kmail_SOURCES = kmmainwin.cpp kmfolderdia.cpp kmfoldertree.cpp \
		kmsettings.cpp kmaccount.cpp kmheaders.cpp \
		kmcomposewin.cpp kmfolder.cpp kmmsgpartdlg.cpp \
		kmmessage.cpp kmreaderwin.cpp kbusyptr.cpp \
		kalarmtimer.cpp kmmsgpart.cpp kmmsginfo.cpp \
		kmacctmgr.cpp main.cpp kmacctfolder.cpp \
		kmacctlocal.cpp kmacctpop.cpp kmfolderdir.cpp \
		kmfoldermgr.cpp kmfoldernode.cpp kmsender.cpp \
		kmacctseldlg.cpp kmimemagic.cpp kmfiltermgr.cpp \
		kmfilteraction.cpp kmidentity.cpp \
		kmfolderseldlg.cpp kmfilter.cpp kmfilterdlg.cpp \
		kmmsgbase.cpp kmmsglist.cpp kpgp.cpp \
		kfileio.cpp kmaddrbook.cpp \
		kmaddrbookdlg.cpp kmiostatusdlg.cpp \
		kmnewiostatus.cpp kmnewiostatuswdg.cpp \
		charsetsDlg.cpp kfontutils.cpp kmtopwidget.cpp

BUILT_SOURCES = kmmainwin.moc kmfolderdia.moc kmfolder.moc \
		kmfoldertree.moc kmsettings.moc kmheaders.moc \
		kmcomposewin.moc kbusyptr.moc kmfoldermgr.moc \
		kalarmtimer.moc kmreaderwin.moc kmsender.moc \
		kmacctmgr.moc kmfolderdir.moc kmfilteraction.moc \
		kmfoldernode.moc kmacctseldlg.moc kmmsgpartdlg.moc \
		kmimemagic.moc kmacctpop.moc kmaccount.moc \
		kmfolderseldlg.moc kmfilterdlg.moc kpgp.moc \
		kmaddrbookdlg.moc kmiostatusdlg.moc \
		kmnewiostatus.moc kmnewiostatuswdg.moc \
		charsetsDlg.moc kmtopwidget.moc

noinst_HEADERS = *.h

DISTCLEANFILES = $(BUILT_SOURCES)

appsdir = $(kde_appsdir)/Internet/

EXTRA_DIST = KMail.kdelnk kmail.xpm mini-kmail.xpm

install-data-local:
	$(mkinstalldirs) $(appsdir)
	$(INSTALL_DATA) KMail.kdelnk $(appsdir)
	$(mkinstalldirs) $(kde_icondir)	
	$(INSTALL_DATA) kmail.xpm $(kde_icondir)/kmail.xpm
	$(mkinstalldirs) $(kde_minidir)
	$(INSTALL_DATA) mini-kmail.xpm $(kde_minidir)/kmail.xpm

uninstall-local:
	rm -f $(appsdir)/KMail.kdelnk
	rm -f $(kde_icondir)/kmail.xpm
	rm -f $(kde_minidir)/kmail.xpm

messages:
	@XGETTEXT@ -ki18n -C -x../../kdelibs/po/kde.pot $(kmail_SOURCES) && mv messages.po ../po/kmail.pot 

kmfolderdia.cpp:kmfolderdia.moc
kmfolderdia.moc: kmfolderdia.h
	$(MOC) kmfolderdia.h -o kmfolderdia.moc

kmfolderseldlg.cpp:kmfolderseldlg.moc
kmfolderseldlg.moc: kmfolderseldlg.h
	$(MOC) kmfolderseldlg.h -o kmfolderseldlg.moc

kmfolder.cpp:kmfolder.moc
kmfolder.moc: kmfolder.h
	$(MOC) kmfolder.h -o kmfolder.moc

kmfoldertree.cpp:kmfoldertree.moc
kmfoldertree.moc: kmfoldertree.h
	$(MOC) kmfoldertree.h -o kmfoldertree.moc

kmacctseldlg.cpp:kmacctseldlg.moc
kmacctseldlg.moc: kmacctseldlg.h
	$(MOC) kmacctseldlg.h -o kmacctseldlg.moc

kmfoldernode.cpp:kmfoldernode.moc
kmfoldernode.moc: kmfoldernode.h
	$(MOC) kmfoldernode.h -o kmfoldernode.moc

kmacctmgr.cpp:kmacctmgr.moc
kmacctmgr.moc: kmacctmgr.h
	$(MOC) kmacctmgr.h -o kmacctmgr.moc

kmsettings.cpp:kmsettings.moc
kmsettings.moc: kmsettings.h
	$(MOC) kmsettings.h -o kmsettings.moc

kmreaderwin.cpp:kmreaderwin.moc
kmreaderwin.moc: kmreaderwin.h
	$(MOC) kmreaderwin.h -o kmreaderwin.moc

kmmainwin.cpp:kmmainwin.moc
kmmainwin.moc: kmmainwin.h
	$(MOC) kmmainwin.h -o kmmainwin.moc

kmheaders.cpp:kmheaders.moc
kmheaders.moc: kmheaders.h
	$(MOC) kmheaders.h -o kmheaders.moc

kmfolderdir.cpp:kmfolderdir.moc
kmfolderdir.moc: kmfolderdir.h
	$(MOC) kmfolderdir.h -o kmfolderdir.moc

kmcomposewin.cpp:kmcomposewin.moc
kmcomposewin.moc: kmcomposewin.h
	$(MOC) kmcomposewin.h -o kmcomposewin.moc

kbusyptr.cpp:kbusyptr.moc
kbusyptr.moc: kbusyptr.h
	$(MOC) kbusyptr.h -o kbusyptr.moc

kalarmtimer.cpp:kalarmtimer.moc
kalarmtimer.moc: kalarmtimer.h
	$(MOC) kalarmtimer.h -o kalarmtimer.moc

kmimemagic.cpp: kmimemagic.moc
kmimemagic.moc: kmimemagic.h
	$(MOC) kmimemagic.h -o kmimemagic.moc

kmacctpop.cpp: kmacctpop.moc
kmacctpop.moc: kmacctpop.h
	$(MOC) kmacctpop.h -o kmacctpop.moc

kmaccount.cpp: kmaccount.moc
kmaccount.moc: kmaccount.h
	$(MOC) kmaccount.h -o kmaccount.moc

kmfilterdlg.cpp: kmfilterdlg.moc
kmfilterdlg.moc: kmfilterdlg.h
	$(MOC) kmfilterdlg.h -o kmfilterdlg.moc

kmfilteraction.cpp: kmfilteraction.moc
kmfilteraction.moc: kmfilteraction.h
	$(MOC) kmfilteraction.h -o kmfilteraction.moc

kmsglist.cpp: kmsglist.moc
kmsglist.moc: kmsglist.h
	$(MOC) kmsglist.h -o kmsglist.moc

kmfoldermgr.cpp: kmfoldermgr.moc
kmfoldermgr.moc: kmfoldermgr.h
	$(MOC) kmfoldermgr.h -o kmfoldermgr.moc

kmmsgpartdlg.cpp: kmmsgpartdlg.moc
kmmsgpartdlg.moc: kmmsgpartdlg.h
	$(MOC) kmmsgpartdlg.h -o kmmsgpartdlg.moc

kpgp.cpp: kpgp.moc
kpgp.moc: kpgp.h
	$(MOC) kpgp.h -o kpgp.moc

kmaddrbookdlg.cpp:kmaddrbookdlg.moc
kmaddrbookdlg.moc: kmaddrbookdlg.h
	$(MOC) kmaddrbookdlg.h -o kmaddrbookdlg.moc

kmsender.cpp: kmsender.moc
kmsender.moc: kmsender.h
	$(MOC) kmsender.h -o kmsender.moc

kmiostatusdlg.cpp: kmiostatusdlg.moc
kmiostatusdlg.moc: kmiostatusdlg.h
	$(MOC) kmiostatusdlg.h -o kmiostatusdlg.moc

kmnewiostatuswdg.cpp: kmnewiostatuswdg.moc
kmnewiostatuswdg.moc: kmnewiostatuswdg.h
	$(MOC) kmnewiostatuswdg.h -o kmnewiostatuswdg.moc

kmnewiostatus.cpp: kmnewiostatus.moc
kmnewiostatus.moc: kmnewiostatus.h
	$(MOC) kmnewiostatus.h -o kmnewiostatus.moc

charsetsDlg.cpp: charsetsDlg.moc
charsetsDlg.moc: charsetsDlg.h
	$(MOC) charsetsDlg.h -o charsetsDlg.moc

kmtopwidget.cpp: kmtopwidget.moc
kmtopwidget.moc: kmtopwidget.h
	$(MOC) kmtopwidget.h -o kmtopwidget.moc
