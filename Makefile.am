
SUBDIRS = pics doc
INCLUDES=	-I.. @all_includes@
LDADD	=	@all_libraries@ -lkdeui -lkdecore -lkhtmlw -ljscript -lqt -lXext -lmimelib
MOC	=	@MOC@

bin_PROGRAMS = kmail

kmail_SOURCES = kmmainwin.cpp kmfolderdia.cpp kmfoldertree.cpp \
		kmsettings.cpp kmaccount.cpp kmheaders.cpp \
		ktablistbox.cpp kmcomposewin.cpp kmfolder.cpp \
		kmmessage.cpp kmreaderwin.cpp kbusyptr.cpp \
		kalarmtimer.cpp KEdit.cpp kmmsgpart.cpp kmmsginfo.cpp \
		kmacctmgr.cpp main.cpp kmmainview.cpp kmacctfolder.cpp \
		kmacctlocal.cpp kmacctpop.cpp kmfolderdir.cpp \
		kmfoldermgr.cpp kmfoldernode.cpp kmsender.cpp \
		kmacctseldlg.cpp kshortcut.cpp

BUILT_SOURCES = kmmainwin.moc kmfolderdia.moc kmfolder.moc \
		kmfoldertree.moc kmsettings.moc kmheaders.moc \
		ktablistbox.moc kmcomposewin.moc kbusyptr.moc \
		kalarmtimer.moc KEdit.moc kmreaderwin.moc \
		kmmainview.moc kmacctmgr.moc kmfolderdir.moc \
		kmfoldernode.moc kmacctseldlg.moc kshortcut.moc

CLEAN_FILES = $(BUILT_SOURCES)

appsdir = $(prefix)/share/applnk/Utilities/

install-data-local:
	$(mkinstalldirs) $(appsdir)
	$(INSTALL_DATA) KMail.kdelnk $(appsdir)

uninstall-local:
	rm -f $(appsdir)/KMail.kdelnk
	
kmfolderdia.cpp:kmfolderdia.moc
kmfolderdia.moc: kmfolderdia.h
	$(MOC) kmfolderdia.h -o kmfolderdia.moc

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

kmmainview.cpp:kmmainview.moc
kmmainview.moc: kmmainview.h
	$(MOC) kmmainview.h -o kmmainview.moc

kmacctmgr.cpp:kmacctmgr.moc
kmacctmgr.moc: kmacctmgr.h
	$(MOC) kmacctmgr.h -o kmacctmgr.moc

kmsettings.cpp:kmsettings.moc
kmsettings.moc: kmsettings.h
	$(MOC) kmsettings.h -o kmsettings.moc

KEdit.cpp:KEdit.moc
KEdit.moc: KEdit.h
	$(MOC) KEdit.h -o KEdit.moc

kmreaderwin.cpp:kmreaderwin.moc
kmreaderwin.moc: kmreaderwin.h
	$(MOC) kmreaderwin.h -o kmreaderwin.moc

kmmessage.cpp:kmmessage.moc
kmmessage.moc: kmmessage.h
	$(MOC) kmmessage.h -o kmmessage.moc

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

ktablistbox.cpp:ktablistbox.moc
ktablistbox.moc: ktablistbox.h
	$(MOC) ktablistbox.h -o ktablistbox.moc

kbusyptr.cpp:kbusyptr.moc
kbusyptr.moc: kbusyptr.h
	$(MOC) kbusyptr.h -o kbusyptr.moc

kalarmtimer.cpp:kalarmtimer.moc
kalarmtimer.moc: kalarmtimer.h
	$(MOC) kalarmtimer.h -o kalarmtimer.moc

kshortcut.cpp: kshortcut.moc
kshortcut.moc: kshortcut.h
	$(MOC) kshortcut.h -o kshortcut.moc

