KDE_CXXFLAGS = $(USE_RTTI) -UQT_NO_ASCII_CAST -UQT_NO_COMPAT

SUBDIRS = about pics
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
		kmacctlocal.cpp kmfolderdir.cpp \
		kmfoldermgr.cpp kmfoldernode.cpp kmsender.cpp \
		kmacctseldlg.cpp kmfiltermgr.cpp \
		kmfilteraction.cpp kmidentity.cpp \
		kmfolderseldlg.cpp kmfilter.cpp kmfilterdlg.cpp \
		kmmsgbase.cpp kmmsglist.cpp kpgp.cpp kpgpbase.cpp\
		kfileio.cpp kmaddrbook.cpp \
		kmaddrbookdlg.cpp kmiostatusdlg.cpp \
		kmnewiostatus.cpp kmnewiostatuswdg.cpp \
		kmtopwidget.cpp kmacctimap.cpp \
		kmundostack.cpp kmbroadcaststatus.cpp \
		kmacctexppop.cpp configuredialog.cpp colorlistbox.cpp \
                kmkernel.cpp kmailIface.skel kmailIface.stub main.cpp \
		accountdialog.cpp kmfldsearch.cpp addtoaddressbook.cpp \
		kmdisplayvcard.cpp vcard.cpp md5.c


include_HEADERS = kmailIface.h

METASOURCES = AUTO

appsdir = $(kde_appsdir)/Internet
apps_DATA = KMail.desktop

EXTRA_DIST = KMail.desktop

KDE_ICON = AUTO

rcdir = $(kde_datadir)/kmail
rc_DATA = kmcomposerui.rc kmmainwin.rc

messages: rc.cpp
	$(XGETTEXT) *.cpp *.h -o $(podir)/kmail.pot

