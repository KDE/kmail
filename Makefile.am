KDE_CXXFLAGS = $(USE_RTTI)

SUBDIRS = . about pics
INCLUDES = -I$(top_srcdir)/libkdenetwork -I$(top_srcdir)/mimelib $(all_includes)
LDADD	= $(LIB_KHTML) ../libkdenetwork/libkdenetwork.la -lkspell ../mimelib/libmimelib.la $(LIB_KAB)

bin_PROGRAMS = kmail

kmail_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kmail_SOURCES = kmmessage.cpp kmmainwin.cpp configuredialog.cpp \
		kmfolderdia.cpp kmfoldertree.cpp \
		kmaccount.cpp kmheaders.cpp \
		kmcomposewin.cpp kmfolder.cpp kmmsgpartdlg.cpp \
		kmreaderwin.cpp kbusyptr.cpp \
		kalarmtimer.cpp kmmsgpart.cpp kmmsginfo.cpp \
		kmacctmgr.cpp kmacctfolder.cpp \
		kmacctlocal.cpp kmfolderdir.cpp \
		kmfoldermgr.cpp kmfoldernode.cpp kmsender.cpp \
		kmacctseldlg.cpp kmfiltermgr.cpp kmsearchpatternedit.cpp \
		kmfilteraction.cpp kmsearchpattern.cpp kmidentity.cpp \
		kmfolderseldlg.cpp kmfilter.cpp kmfilterdlg.cpp \
		kmmsgbase.cpp kmmsglist.cpp kmpgpwrap.cpp \
		kfileio.cpp kmaddrbook.cpp \
		kmaddrbookdlg.cpp kmiostatusdlg.cpp \
		kmnewiostatus.cpp kmnewiostatuswdg.cpp \
		kmtopwidget.cpp kmacctimap.cpp \
		kmundostack.cpp kmbroadcaststatus.cpp \
		kmscoring.cpp kmfawidgets.cpp \
		kmacctexppop.cpp colorlistbox.cpp \
                kmkernel.cpp kmailIface.skel kmailIface.stub main.cpp \
		accountdialog.cpp kmfldsearch.cpp addtoaddressbook.cpp \
		kmdisplayvcard.cpp vcard.cpp \
		smtp.cpp mailinglist-magic.cpp kmrecentaddr.cpp


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

