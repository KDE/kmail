#KDE_OPTIONS = nofinal
KDE_CXXFLAGS = $(USE_RTTI)

SUBDIRS = interfaces . about pics profiles avscripts

INCLUDES = -I$(top_srcdir)/libkmime \
	-I$(top_srcdir)/libkpgp \
	-I$(top_srcdir)/libkdepim \
	-I$(top_srcdir)/libkdenetwork \
	-I$(top_srcdir)/libkpimidentities \
	-I$(top_srcdir)/libksieve \
	-I$(top_srcdir)/mimelib \
	-I$(top_srcdir)/certmanager/lib \
	-I$(top_srcdir) \
	$(GPGME_CFLAGS) \
	$(all_includes)

lib_LTLIBRARIES = libkmailprivate.la
libkmailprivate_la_LDFLAGS = $(all_libraries) -version-info 0:0:0 -no-undefined
libkmailprivate_la_LIBADD  = $(LIB_KHTML) $(LIB_KSPELL) $(LIB_KABC) \
	../libkmime/libkmime.la ../libkpgp/libkpgp.la ../libkdepim/libkdepim.la \
	../libkpimidentities/libkpimidentities.la ../mimelib/libmimelib.la \
	../libksieve/libksieve.la ../libemailfunctions/libemailfunctions.la \
	../certmanager/lib/libkleopatra.la \
	../libkcal/libkcal.la

kde_module_LTLIBRARIES = kcm_kmail.la libkmailpart.la libkmail_bodypartformatter_application_octetstream.la
libkmailpart_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -version-info 1:0 -no-undefined
libkmailpart_la_LIBADD	= libkmailprivate.la

kcm_kmail_la_SOURCES = kcm_kmail.cpp
kcm_kmail_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
kcm_kmail_la_LIBADD = libkmailprivate.la $(LIB_KDECORE)

libkmail_bodypartformatter_application_octetstream_la_SOURCES = app_octetstream.cpp
libkmail_bodypartformatter_application_octetstream_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

linkdir = $(kde_datadir)/kmail/plugins/bodypartformatter
link_DATA = application_octetstream.desktop

bin_PROGRAMS = kmail
kmail_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kmail_LDADD   = libkmailprivate.la

libkmailprivate_la_SOURCES = kmmessage.cpp kmmainwin.cpp configuredialog.cpp \
		configuredialog_p.cpp \
		simplestringlisteditor.cpp kmmsgindex.cpp \
		identitydrag.cpp identitylistview.cpp identitydialog.cpp \
		kmfolderdia.cpp kmfoldertree.cpp kmtransport.cpp \
		kmfoldercombobox.cpp kmaccount.cpp kmheaders.cpp \
		kmcomposewin.cpp kmfolder.cpp kmmsgpartdlg.cpp \
		kmreaderwin.cpp htmlstatusbar.cpp kmmsgdict.cpp \
		kmgroupware.cpp folderstorage.cpp \
		csshelper.cpp klistboxdialog.cpp \
		signatureconfigurationdialog.ui \
		encryptionconfigurationdialog.ui \
		actionscheduler.cpp messageproperty.cpp \
		kmmsgpart.cpp kmmsginfo.cpp \
		kmacctmgr.cpp kmacctfolder.cpp kmdict.cpp \
		kmsystemtray.cpp kmacctlocal.cpp kmfolderdir.cpp \
		kmfoldermgr.cpp kmfoldernode.cpp kmsender.cpp \
		kmacctseldlg.cpp kmfiltermgr.cpp kmsearchpatternedit.cpp \
		kmfilteraction.cpp kmsearchpattern.cpp \
		kmfolderseldlg.cpp kmfilter.cpp kmfilterdlg.cpp \
		kmmsgbase.cpp kmmsglist.cpp kmaddrbook.cpp  \
		signatureconfigurator.cpp \
		networkaccount.cpp imapaccountbase.cpp \
		kmservertest.cpp kmacctimap.cpp kmacctcachedimap.cpp \
		kmfawidgets.cpp kmfoldermbox.cpp kmfolderimap.cpp \
		undostack.cpp kmfoldercachedimap.cpp \
		kmfoldermaildir.cpp kmacctexppop.cpp colorlistbox.cpp \
		kmkernel.cpp kmailIface.skel kmailicalIface.skel \
		accountdialog.cpp kmfldsearch.cpp vcardviewer.cpp \
		vacationdialog.cpp vacation.cpp sieveconfig.cpp sievejob.cpp \
		kmpopheaders.cpp kmpopfiltercnfrmdlg.cpp \
		kmmimeparttree.cpp \
		certificatehandlingdialog.ui \
		certificatehandlingdialogimpl.cpp \
		certificatewizard.ui certificatewizardimpl.cpp \
		mailinglist-magic.cpp kmacctmaildir.cpp \
		attachmentstrategy.cpp \
		headerstrategy.cpp headerstyle.cpp khtmlparthtmlwriter.cpp \
		filehtmlwriter.cpp teehtmlwriter.cpp \
                mailcomposerIface.skel objecttreeparser.cpp \
		attachmentcollector.cpp \
		bodypartformatter.cpp bodypartformatterfactory.cpp \
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
		attachmentlistview.cpp \
		isubject.cpp bodyvisitor.cpp antispamwizard.cpp \
		urlhandlermanager.cpp dictionarycombobox.cpp \
		secondarywindow.cpp filterlog.cpp filterlogdlg.cpp \
		korganizeriface.stub messagecomposer.cpp \
		keyresolver.cpp globalsettings.kcfgc \
		regexplineedit.cpp rulewidgethandlermanager.cpp \
		headerlistquicksearch.cpp acljobs.cpp folderdiaacltab.cpp \
		partnodebodypart.cpp \
		expirejob.cpp compactionjob.cpp jobscheduler.cpp callback.cpp \
		listjob.cpp \
		composercryptoconfiguration.ui \
		warningconfiguration.ui smimeconfiguration.ui annotationjobs.cpp \
		accountcombobox.cpp redirectdialog.cpp foldershortcutdialog.cpp \
                replyphrases.kcfgc custommimeheader.kcfgc

kmail_SOURCES = main.cpp

libkmailpart_la_SOURCES =  kmailpartIface.skel 	kmail_part.cpp

check_PROGRAMS = dcoptest

METASOURCES = AUTO

dcoptest_SOURCES = dcoptest.cpp kmailIface.skel kmailIface.stub mailcomposerIface.skel mailcomposerIface.stub
dcoptest_LDADD = $(LIB_KIO)

kmailIface_DCOPIDLNG = true
kmailicalIface_DCOPIDLNG = true

korganizeriface_DIR = $(top_srcdir)/korganizer

include_HEADERS = kmailIface.h kmailpartIface.h kmailicalIface.h

xdg_apps_DATA = KMail.desktop kmail_view.desktop

# why?
EXTRA_DIST = KMail.desktop $(link_DATA)

KDE_ICON = AUTO

rcdir = $(kde_datadir)/kmail
rc_DATA = kmcomposerui.rc kmmainwin.rc kmreadermainwin.rc eventsrc

updatedir = $(kde_datadir)/kconf_update
update_DATA = kmail.upd
update_SCRIPTS = upgrade-transport.pl kmail-pgpidentity.pl \
		upgrade-signature.pl kmail-upd-identities.pl \
                kmail-3.1-use-UOID-for-identities.pl \
                kmail-3.1-update-new-mail-notification-settings.pl \
                kmail-3.1.4-dont-use-UOID-0-for-any-identity.pl \
		kmail-3.2-update-loop-on-goto-unread-settings.sh \
		kmail-3.2-misc.sh \
		kmail-3.3-use-ID-for-accounts.pl \
		kmail-3.3-move-identities.pl \
		kmail-3.3-aegypten.pl \
		kmail-3.3-split-sign-encr-keys.sh \
		kmail-3.3-misc.pl \
		kmail-3.3b1-misc.pl \
		kmail-3.4-misc.pl 

confdir = $(kde_confdir)
conf_DATA = kmail.antispamrc kmail.antivirusrc

tipdir = $(kde_datadir)/kmail
tip_DATA = tips

servicetypedir = $(kde_servicetypesdir)
servicetype_DATA = dcopmail.desktop dcopimap.desktop

kde_services_DATA = kmail_config_misc.desktop kmail_config_appearance.desktop \
   kmail_config_identity.desktop kmail_config_accounts.desktop kmail_config_composer.desktop \
   kmail_config_security.desktop

messages: rc.cpp
	rm -f tips.cpp
	$(PREPARETIPS) > tips.cpp
	$(XGETTEXT) -ktranslate *.cpp *.h -o $(podir)/kmail.pot
	rm -f tips.cpp

kde_kcfg_DATA = kmail.kcfg replyphrases.kcfg custommimeheader.kcfg

include ../admin/Doxyfile.am
