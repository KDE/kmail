KDE_OPTIONS = nofinal
KDE_CXXFLAGS = $(USE_RTTI)

SUBDIRS =  . about pics profiles

INCLUDES = -I$(top_srcdir)/libkdenetwork \
	-I$(top_srcdir)/libkdepim \
	-I$(top_srcdir)/libksieve \
	-I$(top_srcdir)/mimelib \
	-I$(top_srcdir)/ktnef \
	-I$(top_srcdir) $(all_includes)

lib_LTLIBRARIES = libkmailprivate.la
libkmailprivate_la_LDFLAGS = $(all_libraries) -version-info 0:0:0 -no-undefined
libkmailprivate_la_LIBADD  = $(LIB_KHTML) $(LIB_KSPELL) $(LIB_KABC) \
			../libkdenetwork/libkdenetwork.la ../libkdepim/libkdepim.la \
			../mimelib/libmimelib.la ../ktnef/lib/libktnef.la \
			../libksieve/libksieve.la ../libkcal/libkcal.la

kde_module_LTLIBRARIES = libkmailpart.la
libkmailpart_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -version-info 1:0 -no-undefined
libkmailpart_la_LIBADD	= libkmailprivate.la

bin_PROGRAMS = kmail
kmail_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kmail_LDADD   = libkmailprivate.la

libkmailprivate_la_SOURCES = kmmessage.cpp kmmainwin.cpp configuredialog.cpp \
		configuredialog_p.cpp cryptplugconfigdialog.cpp \
		simplestringlisteditor.cpp kmmsgindex.cpp \
		configmanager.cpp identitymanager.cpp identitycombo.cpp \
		identitydrag.cpp identitylistview.cpp identitydialog.cpp \
		kmfolderdia.cpp kmfoldertree.cpp kmtransport.cpp \
		kmfoldercombobox.cpp kmaccount.cpp kmheaders.cpp \
		kmcomposewin.cpp kmfolder.cpp kmmsgpartdlg.cpp \
		kmreaderwin.cpp htmlstatusbar.cpp kmmsgdict.cpp \
		kmgroupware.cpp kmgroupwarefuncs.cpp \
		csshelper.cpp klistboxdialog.cpp \
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
		kmmsgbase.cpp kmmsglist.cpp kfileio.cpp kmaddrbook.cpp  \
		signatureconfigurator.cpp \
		networkaccount.cpp imapaccountbase.cpp imapprogressdialog.cpp \
		kmtopwidget.cpp kmservertest.cpp kmacctimap.cpp kmacctcachedimap.cpp \
		undostack.cpp kmbroadcaststatus.cpp \
		kmfawidgets.cpp kmfoldermbox.cpp kmfolderimap.cpp \
		kmfoldercachedimap.cpp \
		kmfoldermaildir.cpp kmacctexppop.cpp colorlistbox.cpp \
                kmkernel.cpp kmailIface.skel kmailicalIface.skel \
		accountdialog.cpp kmfldsearch.cpp vcardviewer.cpp \
		vacationdialog.cpp vacation.cpp sieveconfig.cpp sievejob.cpp \
		kmpopheaders.cpp kmpopfiltercnfrmdlg.cpp \
		directoryservicesconfigurationdialogimpl.cpp \
		adddirectoryservicedialog.ui  \
		adddirectoryservicedialogimpl.cpp kmmimeparttree.cpp \
		certificatehandlingdialog.ui \
		certificatehandlingdialogimpl.cpp \
		certificatewizard.ui certificatewizardimpl.cpp \
		mailinglist-magic.cpp kmacctmaildir.cpp \
		attachmentstrategy.cpp \
		headerstrategy.cpp headerstyle.cpp khtmlparthtmlwriter.cpp \
		filehtmlwriter.cpp teehtmlwriter.cpp \
                mailcomposerIface.skel objecttreeparser.cpp \
		bodypartformatter.cpp \
		partNode.cpp signatureconfigurationdialogimpl.cpp \
		encryptionconfigurationdialogimpl.cpp mailsourceviewer.cpp \
		kmcommands.cpp kmreadermainwin.cpp \
		kmstartup.cpp kmmainwidget.cpp \
                folderpropertiesdialog.ui kmfolderindex.cpp \
		kmfoldersearch.cpp transportmanager.cpp \
		folderjob.cpp cachedimapjob.cpp \
		maildirjob.cpp mboxjob.cpp imapjob.cpp \
		subscriptiondialog.cpp kmailicalifaceimpl.cpp aboutdata.cpp \
		folderIface.cpp folderIface.skel mailserviceimpl.cpp \
		attachmentlistview.cpp ssllabel.cpp \
		iobserver.cpp isubject.cpp bodyvisitor.cpp \
		urlhandlermanager.cpp dictionarycombobox.cpp

kmail_SOURCES = main.cpp

libkmailpart_la_SOURCES =  kmailpartIface.skel 	kmail_part.cpp

check_PROGRAMS = dcoptest

METASOURCES = AUTO

dcoptest_SOURCES = dcoptest.cpp kmailIface.skel mailcomposerIface.skel

kmailIface_DCOPIDLNG = true

include_HEADERS = kmailIface.h kmailpartIface.h kmailicalIface.h

xdg_apps_DATA = KMail.desktop

EXTRA_DIST = KMail.desktop

KDE_ICON = AUTO

rcdir = $(kde_datadir)/kmail
rc_DATA = kmcomposerui.rc kmmainwin.rc kmreadermainwin.rc eventsrc

updatedir = $(kde_datadir)/kconf_update
update_DATA = kmail.upd upgrade-transport.pl kmail-pgpidentity.pl \
		upgrade-signature.pl kmail-upd-identities.pl \
                kmail-3.1-use-UOID-for-identities.pl \
                kmail-3.1-update-new-mail-notification-settings.pl \
                kmail-3.1.4-dont-use-UOID-0-for-any-identity.pl \
		kmail-3.2-update-loop-on-goto-unread-settings.sh \
		kmail-3.2-misc.sh

tipdir = $(kde_datadir)/kmail
tip_DATA = tips

servicetypedir = $(kde_servicetypesdir)
servicetype_DATA = dcopmail.desktop

messages: rc.cpp
	rm -f tips.cpp
	$(PREPARETIPS) > tips.cpp
	$(XGETTEXT) -ktranslate *.cpp *.h -o $(podir)/kmail.pot
	rm -f tips.cpp

include ../admin/Doxyfile.am
