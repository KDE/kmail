KDE_OPTIONS = nofinal
KDE_CXXFLAGS = $(USE_RTTI)

SUBDIRS =  . about pics profiles

INCLUDES = -I$(top_srcdir)/libkdenetwork -I$(top_srcdir)/mimelib -I$(top_srcdir)/ktnef -I$(top_srcdir) $(all_includes)

noinst_LTLIBRARIES = libkmailcommon.la

kde_module_LTLIBRARIES = libkmailpart.la
libkmailpart_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -version-info 1:0
libkmailpart_la_LIBADD	= $(LIB_KHTML) $(LIB_KSPELL) $(LIB_KABC) \
			../libkdenetwork/libkdenetwork.la \
			../mimelib/libmimelib.la ../ktnef/lib/libktnef.la libkmailcommon.la

bin_PROGRAMS = kmail
kmail_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kmail_LDADD   = $(LIB_KHTML) $(LIB_KSPELL) $(LIB_KABC) \
		../libkdenetwork/libkdenetwork.la \
		../mimelib/libmimelib.la ../ktnef/lib/libktnef.la libkmailcommon.la

libkmailcommon_la_SOURCES = kmmessage.cpp kmmainwin.cpp configuredialog.cpp \
		configuredialog_p.cpp cryptplugconfigdialog.cpp \
		simplestringlisteditor.cpp kmmsgindex.cpp \
		configmanager.cpp identitymanager.cpp identitycombo.cpp \
		identitydrag.cpp identitylistview.cpp identitydialog.cpp \
		kmfolderdia.cpp kmfoldertree.cpp kmtransport.cpp \
		kmfoldercombobox.cpp kmaccount.cpp kmheaders.cpp \
		kmcomposewin.cpp kmfolder.cpp kmmsgpartdlg.cpp \
		kmreaderwin.cpp kbusyptr.cpp kmmsgdict.cpp \
		kmgroupware.cpp kmgroupwarefuncs.cpp kmgroupwarewizard.cpp \
		klistboxdialog.cpp \
		signatureconfigurationdialog.ui \
		encryptionconfigurationdialog.ui \
		directoryservicesconfigurationdialog.ui \
		kmmsgpart.cpp kmmsginfo.cpp \
		kmacctmgr.cpp kmacctfolder.cpp kmdict.cpp \
		kmsystemtray.cpp kmacctlocal.cpp kmfolderdir.cpp \
		kmfoldermgr.cpp kmfoldernode.cpp kmsender.cpp \
		kmacctseldlg.cpp kmfiltermgr.cpp kmsearchpatternedit.cpp \
		kmfilteraction.cpp kmsearchpattern.cpp kmidentity.cpp \
		kmfolderseldlg.cpp kmfilter.cpp kmfilterdlg.cpp \
		kmmsgbase.cpp kmmsglist.cpp kmpgpwrap.cpp \
		kfileio.cpp kmaddrbook.cpp  \
		signatureconfigurator.cpp \
		networkaccount.cpp imapaccountbase.cpp imapprogressdialog.cpp \
		kmtopwidget.cpp kmservertest.cpp kmacctimap.cpp kmacctcachedimap.cpp \
		kmundostack.cpp kmbroadcaststatus.cpp \
		kmfawidgets.cpp kmfoldermbox.cpp kmfolderimap.cpp \
		kmfoldercachedimap.cpp \
		kmfoldermaildir.cpp kmacctexppop.cpp colorlistbox.cpp \
                kmkernel.cpp kmailIface.skel kmailicalIface.skel \
		accountdialog.cpp kmfldsearch.cpp \
		kmdisplayvcard.cpp vcard.cpp \
		vacationdialog.cpp vacation.cpp sieveconfig.cpp sievejob.cpp \
		kmpopheaders.cpp kmpopfiltercnfrmdlg.cpp \
		directoryservicesconfigurationdialogimpl.cpp \
		adddirectoryservicedialog.ui  \
		adddirectoryservicedialogimpl.cpp kmmimeparttree.cpp \
		certificatehandlingdialog.ui \
		certificatehandlingdialogimpl.cpp \
		certificatewizard.ui certificatewizardimpl.cpp \
		mailinglist-magic.cpp recentaddresses.cpp kmacctmaildir.cpp \
		linklocator.cpp spellingfilter.cpp attachmentstrategy.cpp \
		headerstrategy.cpp headerstyle.cpp khtmlparthtmlwriter.cpp \
		filehtmlwriter.cpp teehtmlwriter.cpp \
                mailcomposerIface.skel objecttreeparser.cpp \
		partNode.cpp signatureconfigurationdialogimpl.cpp \
		encryptionconfigurationdialogimpl.cpp kmtextbrowser.cpp \
		kmcommands.cpp kmreadermainwin.cpp \
		kmstartup.cpp kmmainwidget.cpp \
                folderpropertiesdialog.ui kmfolderindex.cpp \
		kmfoldersearch.cpp transportmanager.cpp \
		folderjob.cpp cachedimapjob.cpp addressesdialog.cpp \
		addresspicker.ui subscriptiondialog.cpp \
		kmailicalifaceimpl.cpp

kmail_SOURCES = main.cpp

libkmailpart_la_SOURCES =  kmailpartIface.skel 	kmail_part.cpp

check_PROGRAMS = dcoptest

METASOURCES = AUTO

noinst_HEADERS = adddirectoryservicedialogimpl.h kmmimeparttree.h \
		 kmmainwin.h certificatehandlingdialogimpl.h \
		 certificatewizardimpl.h partNode.h \
		 kmgroupwarefuncs.h kmgroupwarewizard.h \
		 signatureconfigurationdialogimpl.h \
		 encryptionconfigurationdialogimpl.h \
		 signatureconfigurationdialogimpl.h addressesdialog.h \
		 kmailicalifaceimpl.h

dcoptest_SOURCES = dcoptest.cpp kmailIface.skel mailcomposerIface.skel

include_HEADERS = kmailIface.h kmailpartIface.h kmailicalIface.h

appsdir = $(kde_appsdir)/Internet
apps_DATA = KMail.desktop

EXTRA_DIST = KMail.desktop

KDE_ICON = AUTO

rcdir = $(kde_datadir)/kmail
rc_DATA = kmcomposerui.rc kmmainwin.rc kmreadermainwin.rc eventsrc

partdir   = $(kde_datadir)/kmail
part_DATA = kmmainwin.rc

updatedir = $(kde_datadir)/kconf_update
update_DATA = kmail.upd upgrade-transport.pl kmail-pgpidentity.pl \
		upgrade-signature.pl kmail-upd-identities.pl \
                kmail-3.1-use-UOID-for-identities.pl \
                kmail-3.1-update-new-mail-notification-settings.pl

tipdir = $(kde_datadir)/kmail
tip_DATA = tips

messages: rc.cpp
	rm -f tips.cpp
	$(PREPARETIPS) > tips.cpp
	$(XGETTEXT) *.cpp *.h -o $(podir)/kmail.pot
	rm -f tips.cpp
