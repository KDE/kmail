KDE_CXXFLAGS = $(USE_RTTI) -DQT_NO_COMPAT

SUBDIRS = . about pics profiles
INCLUDES = -I$(top_srcdir)/libkdenetwork -I$(top_srcdir)/mimelib $(all_includes)
LDADD	= $(LIB_KHTML) ../libkdenetwork/libkdenetwork.la -lkspell ../mimelib/libmimelib.la -lkabc

bin_PROGRAMS = kmail

kmail_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kmail_SOURCES = kmmessage.cpp kmmainwin.cpp configuredialog.cpp \
		configuredialog_p.cpp simplestringlisteditor.cpp \
		configmanager.cpp identitymanager.cpp identitycombo.cpp \
		identitydrag.cpp identitylistview.cpp identitydialog.cpp \
		kmfolderdia.cpp kmfoldertree.cpp kmtransport.cpp \
		kmfoldercombobox.cpp kmaccount.cpp kmheaders.cpp \
		kmcomposewin.cpp kmfolder.cpp kmmsgpartdlg.cpp \
		kmreaderwin.cpp kbusyptr.cpp kmmsgdict.cpp \
		klistboxdialog.cpp \
		signatureconfigurationdialog.ui \
		encryptionconfigurationdialog.ui \
		directoryservicesconfigurationdialog.ui \
		kalarmtimer.cpp kmmsgpart.cpp kmmsginfo.cpp \
		kmacctmgr.cpp kmacctfolder.cpp kmdict.cpp \
		kmacctlocal.cpp kmfolderdir.cpp \
		kmfoldermgr.cpp kmfoldernode.cpp kmsender.cpp \
		kmacctseldlg.cpp kmfiltermgr.cpp kmsearchpatternedit.cpp \
		kmfilteraction.cpp kmsearchpattern.cpp kmidentity.cpp \
		kmfolderseldlg.cpp kmfilter.cpp kmfilterdlg.cpp \
		kmmsgbase.cpp kmmsglist.cpp kmpgpwrap.cpp \
		kfileio.cpp kmaddrbook.cpp \
		signatureconfigurator.cpp \
		kmaddrbookdlg.cpp kmiostatusdlg.cpp \
		kmtopwidget.cpp kmservertest.cpp kmacctimap.cpp \
		kmundostack.cpp kmbroadcaststatus.cpp \
		kmscoring.cpp kmfawidgets.cpp kmfoldermbox.cpp kmfolderimap.cpp \
		kmfoldermaildir.cpp kmacctexppop.cpp colorlistbox.cpp \
                kmkernel.cpp kmailIface.skel kmailIface.stub main.cpp \
		accountdialog.cpp kmfldsearch.cpp \
		kmdisplayvcard.cpp vcard.cpp \
		kmpopheaders.cpp kmpopfiltercnfrmdlg.cpp \
		directoryservicesconfigurationdialogimpl.cpp \
		adddirectoryservicedialog.ui adddirectoryservicedialogimpl.cpp kmmimeparttree.cpp \
		certificatehandlingdialog.ui certificatehandlingdialogimpl.cpp \
		certificatewizard.ui certificatewizardimpl.cpp \
		mailinglist-magic.cpp kmrecentaddr.cpp kmacctmaildir.cpp \
		linklocator.cpp spellingfilter.cpp \
                mailcomposerIface.skel mailcomposerIface.stub \
		partNode.cpp signatureconfigurationdialogimpl.cpp \
		encryptionconfigurationdialogimpl.cpp

noinst_HEADERS = directoryservicesconfigurationdialogimpl.h \
		 adddirectoryservicedialogimpl.h kmmimeparttree.h \
		 kmmainwin.h certificatehandlingdialogimpl.h \
		 certificatewizardimpl.h partNode.h \
		 signatureconfigurationdialogimpl.h \
		 encryptionconfigurationdialogimpl.h

check_PROGRAMS = dcoptest

dcoptest_SOURCES = dcoptest.cpp kmailIface.stub mailcomposerIface.stub

include_HEADERS = kmailIface.h

kmail_METASOURCES = AUTO

appsdir = $(kde_appsdir)/Internet
apps_DATA = KMail.desktop

EXTRA_DIST = KMail.desktop

KDE_ICON = AUTO

rcdir = $(kde_datadir)/kmail
rc_DATA = kmcomposerui.rc kmmainwin.rc eventsrc

updatedir = $(kde_datadir)/kconf_update
update_DATA = kmail.upd upgrade-transport.pl kmail-pgpidentity.pl \
		upgrade-signature.pl kmail-upd-identities.pl \
		kmail-3.1-use-UOID-for-identities.pl

tipdir = $(kde_datadir)/kmail
tip_DATA = tips

messages: rc.cpp
	rm -f tips.cpp
	$(PREPARETIPS) > tips.cpp
	$(XGETTEXT) *.cpp *.h -o $(podir)/kmail.pot
	rm -f tips.cpp

