
SUBDIRS = pics
INCLUDES = $(all_includes)
LDADD	= $(LIB_KHTML) -lkspell -lmimelib $(LIB_KFILE)

bin_PROGRAMS = kmail

kmail_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kmail_SOURCES = kmmainwin.cpp kmfolderdia.cpp kmfoldertree.cpp \
		kmsettings.cpp kmaccount.cpp kmheaders.cpp \
		kmcomposewin.cpp kmfolder.cpp kmmsgpartdlg.cpp \
		kmmessage.cpp kmreaderwin.cpp kbusyptr.cpp \
		kalarmtimer.cpp kmmsgpart.cpp kmmsginfo.cpp \
		kmacctmgr.cpp main.cpp kmacctfolder.cpp \
		kmacctlocal.cpp kmacctpop.cpp kmfolderdir.cpp \
		kmfoldermgr.cpp kmfoldernode.cpp kmsender.cpp \
		kmacctseldlg.cpp kmfiltermgr.cpp \
		kmfilteraction.cpp kmidentity.cpp \
		kmfolderseldlg.cpp kmfilter.cpp kmfilterdlg.cpp \
		kmmsgbase.cpp kmmsglist.cpp kpgp.cpp kpgpbase.cpp\
		kfileio.cpp kmaddrbook.cpp \
		kmaddrbookdlg.cpp kmiostatusdlg.cpp \
		kmnewiostatus.cpp kmnewiostatuswdg.cpp \
		charsetsDlg.cpp kfontutils.cpp kmtopwidget.cpp \
		kmundostack.cpp ktablistbox.cpp

METASOURCES = kmmainwin.moc kmfolderdia.moc kmfolder.moc \
		kmfoldertree.moc kmsettings.moc kmheaders.moc \
		kmcomposewin.moc kbusyptr.moc kmfoldermgr.moc \
		kalarmtimer.moc kmreaderwin.moc kmsender.moc \
		kmacctmgr.moc kmfolderdir.moc kmfilteraction.moc \
		kmfoldernode.moc kmacctseldlg.moc kmmsgpartdlg.moc \
		kmacctpop.moc kmaccount.moc \
		kmfolderseldlg.moc kmfilterdlg.moc kpgp.moc \
		kmaddrbookdlg.moc kmiostatusdlg.moc \
		kmnewiostatus.moc kmnewiostatuswdg.moc \
		charsetsDlg.moc kmtopwidget.moc ktablistbox.moc

appsdir = $(kde_appsdir)/Internet
apps_DATA = KMail.desktop

EXTRA_DIST = KMail.desktop

KDE_ICON = kmail

messages:
	$(XGETTEXT) $(kmail_SOURCES) -o $(podir)/kmail.pot

