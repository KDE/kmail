
SUBDIRS = pics
INCLUDES = $(all_includes)
LDADD	= $(LIB_KHTML) -lkspell -lmimelib -lkab $(LIB_KFILE)

bin_PROGRAMS = kmail

kmail_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kmail_SOURCES = kmmessage.cpp kmmainwin.cpp \
		kmfolderdia.cpp kmfoldertree.cpp \
		kmaccount.cpp kmheaders.cpp \
		kmcomposewin.cpp kmfolder.cpp kmmsgpartdlg.cpp \
		kmreaderwin.cpp kbusyptr.cpp \
		kalarmtimer.cpp kmmsgpart.cpp kmmsginfo.cpp \
		kmacctmgr.cpp kmacctfolder.cpp \
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
		kmundostack.cpp kmbroadcaststatus.cpp \
		kmacctexppop.cpp configuredialog.cpp colorlistbox.cpp \
                kmkernel.cpp kmailIface.skel kmailIface.stub main.cpp \
		accountdialog.cpp kmfldsearch.cpp addtoaddressbook.cpp

METASOURCES = kmmainwin.moc kmfolderdia.moc kmfolder.moc \
		kmfoldertree.moc kmheaders.moc \
		kmcomposewin.moc kbusyptr.moc kmfoldermgr.moc \
		kalarmtimer.moc kmreaderwin.moc kmsender.moc \
		kmacctmgr.moc kmfolderdir.moc kmfilteraction.moc \
		kmfoldernode.moc kmacctseldlg.moc kmmsgpartdlg.moc \
		kmacctpop.moc kmaccount.moc \
		kmfolderseldlg.moc kmfilterdlg.moc kpgp.moc \
		kmaddrbookdlg.moc kmiostatusdlg.moc \
		kmnewiostatus.moc kmnewiostatuswdg.moc \
		charsetsDlg.moc kmtopwidget.moc \
		kmbroadcaststatus.moc kmacctexppop.moc configuredialog.moc \
                colorlistbox.moc kmkernel.moc accountdialog.moc


include_HEADERS = kmailIface.h

METASOURCES = AUTO

appsdir = $(kde_appsdir)/Internet
apps_DATA = KMail.desktop

EXTRA_DIST = KMail.desktop

KDE_ICON = AUTO

rcdir = $(kde_datadir)/kmail
rc_DATA = kmcomposerui.rc

messages: rc.cpp
	$(XGETTEXT) *.cpp *.h -o $(podir)/kmail.pot

