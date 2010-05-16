/*
    This file is part of KMail.
    Copyright (c) 2003 Andreas Gungl <a.gungl@gmx.de>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/


#include "antispamwizard.h"
#include "messageviewer/kcursorsaver.h"
#include "kmfilter.h"
#include "kmfilteraction.h"
#include "kmfiltermgr.h"
#include "kmkernel.h"
#include "kmmainwin.h"
#include "folderrequester.h"
#include "foldertreewidget.h"
#include "foldertreeview.h"
#include "readablecollectionproxymodel.h"
#include "util.h"
#include "imapsettings.h"

#include <Akonadi/AgentInstance>

#include <kaction.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <KProcess>
#include <ktoolinvocation.h>
#include <kconfiggroup.h>

#include <QApplication>
#include <qdom.h>
#include <QLabel>
#include <QLayout>
#include <QTimer>
#include <QPixmap>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QBoxLayout>
#include <QVBoxLayout>

using namespace KMail;

AntiSpamWizard::AntiSpamWizard( WizardMode mode,
                                QWidget* parent )
  : KAssistantDialog( parent ),
    mInfoPage( 0 ),
    mSpamRulesPage( 0 ),
    mVirusRulesPage( 0 ),
    mSummaryPage( 0 ),
    mMode( mode )
{
  // read the configuration for the anti-spam tools
  ConfigReader reader( mMode, mToolList );
  reader.readAndMergeConfig();
  mToolList = reader.getToolList();

#ifndef NDEBUG
  if ( mMode == AntiSpam )
    kDebug() << endl <<"Considered anti-spam tools:";
  else
    kDebug() << endl <<"Considered anti-virus tools:";
#endif
  for ( QList<SpamToolConfig>::Iterator it = mToolList.begin();
        it != mToolList.end(); ++it ) {
#ifndef NDEBUG
    kDebug() << "Predefined tool:" << (*it).getId();
    kDebug() << "Config version:" << (*it).getVersion();
    kDebug() << "Selection priority:" << (*it).getPrio();
    kDebug() << "Displayed name:" << (*it).getVisibleName();
    kDebug() << "Executable:" << (*it).getExecutable();
    kDebug() << "WhatsThis URL:" << (*it).getWhatsThisText();
    kDebug() << "Filter name:" << (*it).getFilterName();
    kDebug() << "Detection command:" << (*it).getDetectCmd();
    kDebug() << "Learn spam command:" << (*it).getSpamCmd();
    kDebug() << "Learn ham command:" << (*it).getHamCmd();
    kDebug() << "Detection header:" << (*it).getDetectionHeader();
    kDebug() << "Detection pattern:" << (*it).getDetectionPattern();
    kDebug() << "Use as RegExp:" << (*it).isUseRegExp();
    kDebug() << "Supports Bayes Filter:" << (*it).useBayesFilter();
    kDebug() << "Type:" << (*it).getType();
#endif
  }

  setWindowTitle( ( mMode == AntiSpam ) ? i18n( "Anti-Spam Wizard" )
                                    : i18n( "Anti-Virus Wizard" ) );
  mInfoPage = new ASWizInfoPage( mMode, 0, "" );
  addPage( mInfoPage,
           ( mMode == AntiSpam )
           ? i18n( "Welcome to the KMail Anti-Spam Wizard" )
           : i18n( "Welcome to the KMail Anti-Virus Wizard" ) );
  connect( mInfoPage, SIGNAL( selectionChanged( void ) ),
            this, SLOT( checkProgramsSelections( void ) ) );

  if ( mMode == AntiSpam ) {
    mSpamRulesPage = new ASWizSpamRulesPage( 0, "" );
    addPage( mSpamRulesPage, i18n( "Options to fine-tune the handling of spam messages" ));
    connect( mSpamRulesPage, SIGNAL( selectionChanged( void ) ),
             this, SLOT( slotBuildSummary( void ) ) );
  }
  else {
    mVirusRulesPage = new ASWizVirusRulesPage( 0, "" );
    addPage( mVirusRulesPage, i18n( "Options to fine-tune the handling of virus messages" ));
    connect( mVirusRulesPage, SIGNAL( selectionChanged( void ) ),
             this, SLOT( checkVirusRulesSelections( void ) ) );
  }

  connect( this, SIGNAL( helpClicked( void) ),
            this, SLOT( slotHelpClicked( void ) ) );

  if ( mMode == AntiSpam ) {
    mSummaryPage = new ASWizSummaryPage( 0, "" );
    addPage( mSummaryPage, i18n( "Summary of changes to be made by this wizard" ) );
  }

  QTimer::singleShot( 0, this, SLOT( checkToolAvailability( void ) ) );
}


void AntiSpamWizard::accept()
{
  if ( mSpamRulesPage ) {
    kDebug() << "Folder name for messages classified as spam is"
                    << mSpamRulesPage->selectedSpamCollectionId();
    kDebug() << "Folder name for messages classified as unsure is"
                    << mSpamRulesPage->selectedUnsureCollectionId();
  }
  if ( mVirusRulesPage ) {
    kDebug() << "Folder name for viruses is"
             << mVirusRulesPage->selectedFolderName();
  }

  KMFilterActionDict dict;
  QList<KMFilter*> filterList;
  bool replaceExistingFilters = false;

  // Let's start with virus detection and handling,
  // so we can avoid spam checks for viral messages
  if ( mMode == AntiVirus ) {
    for ( QList<SpamToolConfig>::Iterator it = mToolList.begin();
          it != mToolList.end(); ++it ) {
      if ( mInfoPage->isProgramSelected( (*it).getVisibleName() ) &&
         ( mVirusRulesPage->pipeRulesSelected() && (*it).isVirusTool() ) )
      {
        // pipe messages through the anti-virus tools,
        // one single filter for each tool
        // (could get combined but so it's easier to understand for the user)
        KMFilter* pipeFilter = new KMFilter();
        QList<KMFilterAction*> *pipeFilterActions = pipeFilter->actions();
        KMFilterAction* pipeFilterAction = dict.value( "filter app" )->create();
        pipeFilterAction->argsFromString( (*it).getDetectCmd() );
        pipeFilterActions->append( pipeFilterAction );
        KMSearchPattern* pipeFilterPattern = pipeFilter->pattern();
        pipeFilterPattern->setName( uniqueNameFor( (*it).getFilterName() ) );
        pipeFilterPattern->append( KMSearchRule::createInstance( "<size>",
                                   KMSearchRule::FuncIsGreaterOrEqual, "0" ) );
        pipeFilter->setApplyOnOutbound( false);
        pipeFilter->setApplyOnInbound();
        pipeFilter->setApplyOnExplicit();
        pipeFilter->setStopProcessingHere( false );
        pipeFilter->setConfigureShortcut( false );

        filterList.append( pipeFilter );
      }
    }

    if ( mVirusRulesPage->moveRulesSelected() )
    {
      // Sort out viruses depending on header fields set by the tools
      KMFilter* virusFilter = new KMFilter();
      QList<KMFilterAction*> *virusFilterActions = virusFilter->actions();
      KMFilterAction* virusFilterAction1 = dict.value( "transfer" )->create();
      virusFilterAction1->argsFromString( mVirusRulesPage->selectedFolderName() );
      virusFilterActions->append( virusFilterAction1 );
      if ( mVirusRulesPage->markReadRulesSelected() ) {
        KMFilterAction* virusFilterAction2 = dict.value( "set status" )->create();
        virusFilterAction2->argsFromString( "R" ); // Read
        virusFilterActions->append( virusFilterAction2 );
      }
      KMSearchPattern* virusFilterPattern = virusFilter->pattern();
      virusFilterPattern->setName( uniqueNameFor( i18n( "Virus handling" ) ) );
      virusFilterPattern->setOp( KMSearchPattern::OpOr );
      for ( QList<SpamToolConfig>::Iterator it = mToolList.begin();
            it != mToolList.end(); ++it ) {
        if ( mInfoPage->isProgramSelected( (*it).getVisibleName() ))
        {
          if ( (*it).isVirusTool() )
          {
              const QByteArray header = (*it).getDetectionHeader().toAscii();
              const QString & pattern = (*it).getDetectionPattern();
              if ( (*it).isUseRegExp() )
                virusFilterPattern->append(
                  KMSearchRule::createInstance( header,
                  KMSearchRule::FuncRegExp, pattern ) );
              else
                virusFilterPattern->append(
                  KMSearchRule::createInstance( header,
                  KMSearchRule::FuncContains, pattern ) );
          }
        }
      }
      virusFilter->setApplyOnOutbound( false);
      virusFilter->setApplyOnInbound();
      virusFilter->setApplyOnExplicit();
      virusFilter->setStopProcessingHere( true );
      virusFilter->setConfigureShortcut( false );

      filterList.append( virusFilter );
    }
  }
  else { // AntiSpam mode
    // TODO Existing filters with same name are replaced. This is hardcoded
    // ATM and needs to be replaced with a value from a (still missing)
    // checkbox in the GUI. At least, the replacement is announced in the GUI.
    replaceExistingFilters = true;
    for ( QList<SpamToolConfig>::Iterator it = mToolList.begin();
          it != mToolList.end(); ++it ) {
      if ( mInfoPage->isProgramSelected( (*it).getVisibleName() ) &&
         (*it).isSpamTool() && !(*it).isDetectionOnly() )
      {
        // pipe messages through the anti-spam tools,
        // one single filter for each tool
        // (could get combined but so it's easier to understand for the user)
        KMFilter* pipeFilter = new KMFilter();
        QList<KMFilterAction*> *pipeFilterActions = pipeFilter->actions();
        KMFilterAction* pipeFilterAction = dict.value( "filter app" )->create();
        pipeFilterAction->argsFromString( (*it).getDetectCmd() );
        pipeFilterActions->append( pipeFilterAction );
        KMSearchPattern* pipeFilterPattern = pipeFilter->pattern();
        if ( replaceExistingFilters )
          pipeFilterPattern->setName( (*it).getFilterName() );
        else
          pipeFilterPattern->setName( uniqueNameFor( (*it).getFilterName() ) );
        pipeFilterPattern->append( KMSearchRule::createInstance( "<size>",
                                   KMSearchRule::FuncIsLessOrEqual, "256000" ) );
        pipeFilter->setApplyOnOutbound( false);
        pipeFilter->setApplyOnInbound();
        pipeFilter->setApplyOnExplicit();
        pipeFilter->setStopProcessingHere( false );
        pipeFilter->setConfigureShortcut( false );

        filterList.append( pipeFilter );
      }
    }

    // Sort out spam depending on header fields set by the tools
    KMFilter* spamFilter = new KMFilter();
    QList<KMFilterAction*> *spamFilterActions = spamFilter->actions();
    if ( mSpamRulesPage->moveSpamSelected() )
    {
      KMFilterAction* spamFilterAction1 = dict.value( "transfer" )->create();
      spamFilterAction1->argsFromString( mSpamRulesPage->selectedSpamCollectionId() );
      spamFilterActions->append( spamFilterAction1 );
    }
    KMFilterAction* spamFilterAction2 = dict.value( "set status" )->create();
    spamFilterAction2->argsFromString( "P" ); // Spam
    spamFilterActions->append( spamFilterAction2 );
    if ( mSpamRulesPage->markAsReadSelected() ) {
      KMFilterAction* spamFilterAction3 = dict.value( "set status" )->create();
      spamFilterAction3->argsFromString( "R" ); // Read
      spamFilterActions->append( spamFilterAction3 );
    }
    KMSearchPattern* spamFilterPattern = spamFilter->pattern();
    if ( replaceExistingFilters )
      spamFilterPattern->setName( i18n( "Spam Handling" ) );
    else
      spamFilterPattern->setName( uniqueNameFor( i18n( "Spam Handling" ) ) );
    spamFilterPattern->setOp( KMSearchPattern::OpOr );
    for ( QList<SpamToolConfig>::Iterator it = mToolList.begin();
          it != mToolList.end(); ++it ) {
      if ( mInfoPage->isProgramSelected( (*it).getVisibleName() ) )
      {
          if ( (*it).isSpamTool() )
          {
            const QByteArray header = (*it).getDetectionHeader().toAscii();
            const QString & pattern = (*it).getDetectionPattern();
            if ( (*it).isUseRegExp() )
              spamFilterPattern->append(
                KMSearchRule::createInstance( header,
                KMSearchRule::FuncRegExp, pattern ) );
            else
              spamFilterPattern->append(
                KMSearchRule::createInstance( header,
                KMSearchRule::FuncContains, pattern ) );
          }
      }
    }
    spamFilter->setApplyOnOutbound( false);
    spamFilter->setApplyOnInbound();
    spamFilter->setApplyOnExplicit();
    spamFilter->setStopProcessingHere( true );
    spamFilter->setConfigureShortcut( false );
    filterList.append( spamFilter );

    if ( mSpamRulesPage->moveUnsureSelected() )
    {
      // Sort out messages classified as unsure
      bool atLeastOneUnsurePattern = false;
      KMFilter* unsureFilter = new KMFilter();
      QList<KMFilterAction*> *unsureFilterActions = unsureFilter->actions();
      KMFilterAction* unsureFilterAction1 = dict.value( "transfer" )->create();
      unsureFilterAction1->argsFromString( mSpamRulesPage->selectedUnsureCollectionId() );
      unsureFilterActions->append( unsureFilterAction1 );
      KMSearchPattern* unsureFilterPattern = unsureFilter->pattern();
      if ( replaceExistingFilters )
        unsureFilterPattern->setName( i18n( "Semi spam (unsure) handling" ) );
      else
        unsureFilterPattern->setName( uniqueNameFor( i18n( "Semi spam (unsure) handling" ) ) );
      unsureFilterPattern->setOp( KMSearchPattern::OpOr );
      for ( QList<SpamToolConfig>::Iterator it = mToolList.begin();
            it != mToolList.end(); ++it ) {
        if ( mInfoPage->isProgramSelected( (*it).getVisibleName() ) )
        {
            if ( (*it).isSpamTool() && (*it).hasTristateDetection())
            {
              atLeastOneUnsurePattern = true;
              const QByteArray header = (*it).getDetectionHeader().toAscii();
              const QString & pattern = (*it).getDetectionPattern2();
              if ( (*it).isUseRegExp() )
                unsureFilterPattern->append(
                  KMSearchRule::createInstance( header,
                  KMSearchRule::FuncRegExp, pattern ) );
              else
                unsureFilterPattern->append(
                  KMSearchRule::createInstance( header,
                  KMSearchRule::FuncContains, pattern ) );
            }
        }
      }
      unsureFilter->setApplyOnOutbound( false);
      unsureFilter->setApplyOnInbound();
      unsureFilter->setApplyOnExplicit();
      unsureFilter->setStopProcessingHere( true );
      unsureFilter->setConfigureShortcut( false );

      if ( atLeastOneUnsurePattern )
        filterList.append( unsureFilter );
      else
        delete unsureFilter;
    }

    // Classify messages manually as Spam
    KMFilter* classSpamFilter = new KMFilter();
    classSpamFilter->setIcon( "mail-mark-junk" );
    QList<KMFilterAction*> *classSpamFilterActions = classSpamFilter->actions();
    KMFilterAction* classSpamFilterActionFirst = dict.value( "set status" )->create();
    classSpamFilterActionFirst->argsFromString( "P" );
    classSpamFilterActions->append( classSpamFilterActionFirst );
    for ( QList<SpamToolConfig>::Iterator it = mToolList.begin();
          it != mToolList.end(); ++it ) {
      if ( mInfoPage->isProgramSelected( (*it).getVisibleName() )
          && (*it).useBayesFilter() && !(*it).isDetectionOnly() )
      {
        KMFilterAction* classSpamFilterAction = dict.value( "execute" )->create();
        classSpamFilterAction->argsFromString( (*it).getSpamCmd() );
        classSpamFilterActions->append( classSpamFilterAction );
      }
    }
    if ( mSpamRulesPage->moveSpamSelected() )
    {
      KMFilterAction* classSpamFilterActionLast = dict.value( "transfer" )->create();
      classSpamFilterActionLast->argsFromString( mSpamRulesPage->selectedSpamCollectionId() );
      classSpamFilterActions->append( classSpamFilterActionLast );
    }

    KMSearchPattern* classSpamFilterPattern = classSpamFilter->pattern();
    if ( replaceExistingFilters )
      classSpamFilterPattern->setName( i18n( "Classify as Spam" ) );
    else
      classSpamFilterPattern->setName( uniqueNameFor( i18n( "Classify as Spam" ) ) );
    classSpamFilterPattern->append( KMSearchRule::createInstance( "<size>",
                                    KMSearchRule::FuncIsGreaterOrEqual, "0" ) );
    classSpamFilter->setApplyOnOutbound( false);
    classSpamFilter->setApplyOnInbound( false );
    classSpamFilter->setApplyOnExplicit( false );
    classSpamFilter->setStopProcessingHere( true );
    classSpamFilter->setConfigureShortcut( true );
    classSpamFilter->setConfigureToolbar( true );
    classSpamFilter->setToolbarName( i18n( "Spam" ) );
    filterList.append( classSpamFilter );

    // Classify messages manually as not Spam / as Ham
    KMFilter* classHamFilter = new KMFilter();
    classHamFilter->setIcon( "mail-mark-notjunk" );
    QList<KMFilterAction*> *classHamFilterActions = classHamFilter->actions();
    KMFilterAction* classHamFilterActionFirst = dict.value( "set status" )->create();
    classHamFilterActionFirst->argsFromString( "H" );
    classHamFilterActions->append( classHamFilterActionFirst );
    for ( QList<SpamToolConfig>::Iterator it = mToolList.begin();
          it != mToolList.end(); ++it ) {
      if ( mInfoPage->isProgramSelected( (*it).getVisibleName() )
          && (*it).useBayesFilter() && !(*it).isDetectionOnly() )
      {
        KMFilterAction* classHamFilterAction = dict.value( "execute" )->create();
        classHamFilterAction->argsFromString( (*it).getHamCmd() );
        classHamFilterActions->append( classHamFilterAction );
      }
    }
    for ( QList<SpamToolConfig>::iterator it = mToolList.begin();
          it != mToolList.end(); ++it ) {
      if ( mInfoPage->isProgramSelected( (*it).getVisibleName() )
          && (*it).useBayesFilter() && !(*it).isDetectionOnly() )
      {
        KMFilterAction* classHamFilterAction = dict.value( "filter app" )->create();
        classHamFilterAction->argsFromString( (*it).getNoSpamCmd() );
        classHamFilterActions->append( classHamFilterAction );
      }
    }
    KMSearchPattern* classHamFilterPattern = classHamFilter->pattern();
    if ( replaceExistingFilters )
      classHamFilterPattern->setName( i18n( "Classify as NOT Spam" ) );
    else
      classHamFilterPattern->setName( uniqueNameFor( i18n( "Classify as NOT Spam" ) ) );
    classHamFilterPattern->append( KMSearchRule::createInstance( "<size>",
                                    KMSearchRule::FuncIsGreaterOrEqual, "0" ) );
    classHamFilter->setApplyOnOutbound( false);
    classHamFilter->setApplyOnInbound( false );
    classHamFilter->setApplyOnExplicit( false );
    classHamFilter->setStopProcessingHere( true );
    classHamFilter->setConfigureShortcut( true );
    classHamFilter->setConfigureToolbar( true );
    classHamFilter->setToolbarName( i18n( "Ham" ) );
    filterList.append( classHamFilter );
  }

  /* Now that all the filters have been added to the list, tell
   * the filter manager about it. That will emit filterListUpdate
   * which will result in the filter list in kmmainwidget being
   * initialized. This should happend only once. */
  if ( !filterList.isEmpty() )
    KMKernel::self()->filterMgr()->appendFilters(
          filterList, replaceExistingFilters );

  KDialog::accept();
}


void AntiSpamWizard::checkProgramsSelections()
{
  bool status = false;
  bool supportUnsure = false;

  mSpamToolsUsed = false;
  mVirusToolsUsed = false;
  for ( QList<SpamToolConfig>::Iterator it = mToolList.begin();
        it != mToolList.end(); ++it ) {
    if ( mInfoPage->isProgramSelected( (*it).getVisibleName() ) )
    {
      status = true;
      if ( (*it).isSpamTool() ) {
        mSpamToolsUsed = true;
        if ( (*it).hasTristateDetection() )
          supportUnsure = true;
      }
      if ( (*it).isVirusTool() )
        mVirusToolsUsed = true;
    }
  }

  if ( mMode == AntiSpam ) {
    mSpamRulesPage->allowUnsureFolderSelection( supportUnsure );
    slotBuildSummary();
  }

  if ( ( mMode == AntiVirus ) && mVirusToolsUsed )
    checkVirusRulesSelections();

  //setNextEnabled( mInfoPage, status );
}


void AntiSpamWizard::checkVirusRulesSelections()
{
  //setFinishEnabled( mVirusRulesPage, anyVirusOptionChecked() );
}


void AntiSpamWizard::checkToolAvailability()
{
  // this can take some time to find the tools
  MessageViewer::KCursorSaver busy( MessageViewer::KBusyPtr::busy() );

  bool found = false;
  for ( QList<SpamToolConfig>::Iterator it = mToolList.begin();
        it != mToolList.end(); ++it ) {
    QString text( i18n("Scanning for %1...", (*it).getId() ) );
    mInfoPage->setScanProgressText( text );
    if ( (*it).isSpamTool() && (*it).isServerBased() ) {
      // check the configured account for pattern in <server>
      QString pattern = (*it).getServerPattern();
      kDebug() << "Testing for server pattern:" << pattern;
      Akonadi::AgentInstance::List lst = KMail::Util::agentInstances();
      foreach( Akonadi::AgentInstance type, lst ) {
        if ( type.identifier().contains( IMAP_RESOURCE_IDENTIFIER ) ) {
          OrgKdeAkonadiImapSettingsInterface *iface = KMail::Util::createImapSettingsInterface( type.identifier() );
          if ( iface->isValid() ) {
            QString host = iface->imapServer();
            mInfoPage->addAvailableTool( (*it).getVisibleName() );
            found = true;
          }
          delete iface;
        }
#if 0
        else if ( type.identifier().contains( POP3_RESOURCE_IDENTIFIER ) ) {
          //TODO look at pop3 resources.
        }
#endif

      }
    }
    else {
      // check the availability of the application
      qApp->processEvents( QEventLoop::ExcludeUserInputEvents, 200 );
      if ( !checkForProgram( (*it).getExecutable() ) ) {
        mInfoPage->addAvailableTool( (*it).getVisibleName() );
        found = true;
      }
    }
  }
  if ( found )
    mInfoPage->setScanProgressText( ( mMode == AntiSpam )
                                    ? i18n("Scanning for anti-spam tools finished.")
                                    : i18n("Scanning for anti-virus tools finished.") );
  else
    mInfoPage->setScanProgressText( ( mMode == AntiSpam )
                                    ? i18n("<p>Sorry, no spam detection tools have been found. "
                                           "Install your spam detection software and "
                                           "re-run this wizard.</p>")
                                    : i18n("Scanning complete. No anti-virus tools found.") );
}


void AntiSpamWizard::slotHelpClicked()
{
  if ( mMode == AntiSpam )
    KToolInvocation::invokeHelp( "the-anti-spam-wizard", "kmail" );
  else
    KToolInvocation::invokeHelp( "the-anti-virus-wizard", "kmail" );
}


void AntiSpamWizard::slotBuildSummary()
{
  QString text;
  QString newFilters;
  QString replaceFilters;

  if ( mMode == AntiVirus ) {
    text = ""; // TODO add summary for the virus part
  }
  else { // AntiSpam mode
    if ( mSpamRulesPage->markAsReadSelected() ) {
      if ( mSpamRulesPage->moveSpamSelected() )
        text = i18n( "<p>Messages classified as spam are marked as read."
                     "<br />Spam messages are moved into the folder named <i>%1</i>.</p>"
                     , mSpamRulesPage->selectedSpamCollectionName() );
      else
        text = i18n( "<p>Messages classified as spam are marked as read."
                     "<br />Spam messages are not moved into a certain folder.</p>" );
    }
    else {
      if ( mSpamRulesPage->moveSpamSelected() )
        text = i18n( "<p>Messages classified as spam are not marked as read."
                     "<br />Spam messages are moved into the folder named <i>%1</i>.</p>"
                     , mSpamRulesPage->selectedSpamCollectionName() );
      else
        text = i18n( "<p>Messages classified as spam are not marked as read."
                     "<br />Spam messages are not moved into a certain folder.</p>" );
    }

    for ( QList<SpamToolConfig>::Iterator it = mToolList.begin();
          it != mToolList.end(); ++it ) {
      if ( mInfoPage->isProgramSelected( (*it).getVisibleName() ) &&
         (*it).isSpamTool() && !(*it).isDetectionOnly() ) {
        sortFilterOnExistance( (*it).getFilterName(), newFilters, replaceFilters );
      }
    }
    sortFilterOnExistance( i18n( "Spam Handling" ), newFilters, replaceFilters );

    // The need for a andling of status "probably spam" depends on the tools chosen
    if ( mSpamRulesPage->moveUnsureSelected() ) {
      bool atLeastOneUnsurePattern = false;
      for ( QList<SpamToolConfig>::Iterator it = mToolList.begin();
            it != mToolList.end(); ++it ) {
        if ( mInfoPage->isProgramSelected( (*it).getVisibleName() ) ) {
            if ( (*it).isSpamTool() && (*it).hasTristateDetection())
              atLeastOneUnsurePattern = true;
        }
      }
      if ( atLeastOneUnsurePattern ) {
        sortFilterOnExistance( i18n( "Semi spam (unsure) handling" ),
                               newFilters, replaceFilters );
        text += i18n( "<p>The folder for messages classified as unsure (probably spam) is <i>%1</i>.</p>"
          , mSpamRulesPage->selectedUnsureCollectionName() );
      }
    }

    // Manual classification via toolbar icon / manually applied filter action
    sortFilterOnExistance( i18n( "Classify as Spam" ),
                            newFilters, replaceFilters );
    sortFilterOnExistance( i18n( "Classify as NOT Spam" ),
                            newFilters, replaceFilters );

    // Show the filters in the summary
    if ( !newFilters.isEmpty() )
      text += i18n( "<p>The wizard will create the following filters:<ul>%1</ul></p>"
                , newFilters );
    if ( !replaceFilters.isEmpty() )
      text += i18n( "<p>The wizard will replace the following filters:<ul>%1</ul></p>"
                , replaceFilters );
  }

  mSummaryPage->setSummaryText( text );
}


int AntiSpamWizard::checkForProgram( const QString &executable )
{
  kDebug() << "Testing for executable:" << executable;
  KProcess process;
  process.setShellCommand( executable );
  return process.execute ();
}


bool AntiSpamWizard::anyVirusOptionChecked()
{
  return ( mVirusRulesPage->moveRulesSelected()
           || mVirusRulesPage->pipeRulesSelected() );
}


const QString AntiSpamWizard::uniqueNameFor( const QString & name )
{
  return KMKernel::self()->filterMgr()->createUniqueName( name );
}


void AntiSpamWizard::sortFilterOnExistance(
        const QString & intendedFilterName,
        QString & newFilters, QString & replaceFilters )
{
  if ( uniqueNameFor( intendedFilterName ) == intendedFilterName )
    newFilters += "<li>" + intendedFilterName + "</li>";
  else
    replaceFilters += "<li>" + intendedFilterName + "</li>";
}


//---------------------------------------------------------------------------
AntiSpamWizard::SpamToolConfig::SpamToolConfig( const QString &toolId,
      int configVersion, int prio, const QString &name, const QString &exec,
      const QString &url, const QString &filter, const QString &detection,
      const QString &spam, const QString &ham, const QString &noSpam,
      const QString &header, const QString &pattern, const QString &pattern2,
      const QString &serverPattern, bool detectionOnly, bool regExp,
      bool bayesFilter, bool tristateDetection, WizardMode type )
  : mId( toolId ), mVersion( configVersion ), mPrio( prio ),
    mVisibleName( name ), mExecutable( exec ), mWhatsThisText( url ),
    mFilterName( filter ), mDetectCmd( detection ), mSpamCmd( spam ),
    mHamCmd( ham ), mNoSpamCmd( noSpam ), mDetectionHeader( header ),
    mDetectionPattern( pattern ), mDetectionPattern2( pattern2 ),
    mServerPattern( serverPattern ), mDetectionOnly( detectionOnly ),
    mUseRegExp( regExp ), mSupportsBayesFilter( bayesFilter ),
    mSupportsUnsure( tristateDetection ), mType( type )
{
}


bool AntiSpamWizard::SpamToolConfig::isServerBased() const
{
  return !mServerPattern.isEmpty();
}


//---------------------------------------------------------------------------
AntiSpamWizard::ConfigReader::ConfigReader( WizardMode mode,
                                            QList<SpamToolConfig> & configList )
  : mToolList( configList ),
    mMode( mode )
{
  if ( mMode == AntiSpam )
    mConfig = KSharedConfig::openConfig( "kmail.antispamrc" );
  else
    mConfig = KSharedConfig::openConfig( "kmail.antivirusrc" );
}

AntiSpamWizard::ConfigReader::~ConfigReader( )
{
}


void AntiSpamWizard::ConfigReader::readAndMergeConfig()
{
  QString groupName = ( mMode == AntiSpam )
                      ? QString("Spamtool #%1")
                      : QString("Virustool #%1");
  // read the configuration from the global config file
  mConfig->setReadDefaults( true );
  KConfigGroup general( mConfig, "General" );
  int registeredTools = general.readEntry( "tools", 0 );
  for (int i = 1; i <= registeredTools; i++)
  {
    KConfigGroup toolConfig( mConfig, groupName.arg( i ) );
    if( !toolConfig.readEntry( "HeadersOnly", false ) )
      mToolList.append( readToolConfig( toolConfig ) );
  }

  // read the configuration from the user config file
  // and merge newer config data
  mConfig->setReadDefaults( false );
  KConfigGroup user_general( mConfig, "General" );
  int user_registeredTools = user_general.readEntry( "tools", 0 );
  for (int i = 1; i <= user_registeredTools; i++)
  {
    KConfigGroup toolConfig( mConfig, groupName.arg( i ) );
    if( !toolConfig.readEntry( "HeadersOnly", false ) )
      mergeToolConfig( readToolConfig( toolConfig ) );
  }
  // Make sure to have add least one tool listed even when the
  // config file was not found or whatever went wrong
  // Currently only works for spam tools
  if ( mMode == AntiSpam ) {
    if ( registeredTools < 1 && user_registeredTools < 1 )
      mToolList.append( createDummyConfig() );
    sortToolList();
  }
}


AntiSpamWizard::SpamToolConfig
    AntiSpamWizard::ConfigReader::readToolConfig( KConfigGroup & configGroup )
{
  QString id = configGroup.readEntry( "Ident" );
  int version = configGroup.readEntry( "Version", 0 );
#ifndef NDEBUG
  kDebug() << "Found predefined tool:" << id;
  kDebug() << "With config version  :" << version;
#endif
  int prio = configGroup.readEntry( "Priority", 1 );
  QString name = configGroup.readEntry( "VisibleName" );
  QString executable = configGroup.readEntry( "Executable" );
  QString url = configGroup.readEntry( "URL" );
  QString filterName = configGroup.readEntry( "PipeFilterName" );
  QString detectCmd = configGroup.readEntry( "PipeCmdDetect" );
  QString spamCmd = configGroup.readEntry( "ExecCmdSpam" );
  QString hamCmd = configGroup.readEntry( "ExecCmdHam" );
  QString noSpamCmd = configGroup.readEntry( "PipeCmdNoSpam" );
  QString header = configGroup.readEntry( "DetectionHeader" );
  QString pattern = configGroup.readEntry( "DetectionPattern" );
  QString pattern2 = configGroup.readEntry( "DetectionPattern2" );
  QString serverPattern = configGroup.readEntry( "ServerPattern" );
  bool detectionOnly = configGroup.readEntry( "DetectionOnly", false );
  bool useRegExp = configGroup.readEntry( "UseRegExp", false );
  bool supportsBayes = configGroup.readEntry( "SupportsBayes", false );
  bool supportsUnsure = configGroup.readEntry( "SupportsUnsure", false );
  return SpamToolConfig( id, version, prio, name, executable, url,
                         filterName, detectCmd, spamCmd, hamCmd, noSpamCmd,
                         header, pattern, pattern2, serverPattern,
                         detectionOnly, useRegExp,
                         supportsBayes, supportsUnsure, mMode );
}


AntiSpamWizard::SpamToolConfig AntiSpamWizard::ConfigReader::createDummyConfig()
{
  return SpamToolConfig( "spamassassin", 0, 1,
                        "SpamAssassin", "spamassassin -V",
                        "http://spamassassin.org", "SpamAssassin Check",
                        "spamassassin -L",
                        "sa-learn -L --spam --no-sync --single",
                        "sa-learn -L --ham --no-sync --single",
                        "spamassassin -d",
                        "X-Spam-Flag", "yes", "", "",
                        false, false, true, false, AntiSpam );
}


void AntiSpamWizard::ConfigReader::mergeToolConfig( AntiSpamWizard::SpamToolConfig config )
{
  bool found = false;
  for ( QList<SpamToolConfig>::Iterator it = mToolList.begin();
        it != mToolList.end(); ++it ) {
#ifndef NDEBUG
    kDebug() << "Check against tool:" << (*it).getId();
    kDebug() << "Against version   :" << (*it).getVersion();
#endif
    if ( (*it).getId() == config.getId() )
    {
      found = true;
      if ( (*it).getVersion() < config.getVersion() )
      {
#ifndef NDEBUG
        kDebug() << "Replacing config ...";
#endif
        mToolList.erase( it );
        mToolList.append( config );
      }
      break;
    }
  }
  if ( !found )
    mToolList.append( config );
}


void AntiSpamWizard::ConfigReader::sortToolList()
{
  QList<SpamToolConfig> tmpList;
  SpamToolConfig config;

  while ( !mToolList.isEmpty() ) {
    QList<SpamToolConfig>::Iterator highest;
    int priority = 0; // ascending
    for ( QList<SpamToolConfig>::Iterator it = mToolList.begin();
          it != mToolList.end(); ++it ) {
      if ( (*it).getPrio() > priority ) {
        priority = (*it).getPrio();
        highest = it;
      }
    }
    config = (*highest);
    tmpList.append( config );
    mToolList.erase( highest );
  }
  for ( QList<SpamToolConfig>::Iterator it = tmpList.begin();
        it != tmpList.end(); ++it ) {
    mToolList.append( (*it) );
  }
}


//---------------------------------------------------------------------------
ASWizPage::ASWizPage( QWidget * parent, const char * name,
                      const QString *bannerName )
  : QWidget( parent )
{
  setObjectName( name );
  QString banner = "kmwizard.png";
  if ( bannerName && !bannerName->isEmpty() )
    banner = *bannerName;

  mLayout = new QHBoxLayout( this );
  mLayout->setSpacing( KDialog::spacingHint() );
  mLayout->setMargin( KDialog::marginHint() );

  QVBoxLayout * sideLayout = new QVBoxLayout();
  mLayout->addItem( sideLayout );
  mLayout->addItem( new QSpacerItem( 5, 5, QSizePolicy::Minimum, QSizePolicy::Expanding ) );

  mBannerLabel = new QLabel( this );
  mBannerLabel->setPixmap( UserIcon(banner) );
  mBannerLabel->setScaledContents( false );
  mBannerLabel->setFrameShape( QFrame::StyledPanel );
  mBannerLabel->setFrameShadow( QFrame::Sunken );
  mBannerLabel->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

  sideLayout->addWidget( mBannerLabel );
  sideLayout->addItem( new QSpacerItem( 5, 5, QSizePolicy::Minimum, QSizePolicy::Expanding ) );
}


//---------------------------------------------------------------------------
ASWizInfoPage::ASWizInfoPage( AntiSpamWizard::WizardMode mode,
                              QWidget * parent, const char * name )
  : ASWizPage( parent, name )
{
  QBoxLayout * layout = new QVBoxLayout();
  mLayout->addItem( layout );

  mIntroText = new QLabel( this );
  mIntroText->setText(
    ( mode == AntiSpamWizard::AntiSpam )
    ? i18n(
      "The wizard will search for any tools to do spam detection\n"
      "and setup KMail to work with them."
      )
    : i18n(
      "<p>Here you can get some assistance in setting up KMail's filter "
      "rules to use some commonly-known anti-virus tools.</p>"
      "<p>The wizard can detect those tools on your computer as "
      "well as create filter rules to classify messages using these "
      "tools and to separate messages containing viruses. "
      "The wizard will not take any existing filter "
      "rules into consideration: it will always append the new rules.</p>"
      "<p><b>Warning:</b> As KMail appears to be frozen during the scan of the "
      "messages for viruses, you may encounter problems with "
      "the responsiveness of KMail because anti-virus tool "
      "operations are usually time consuming; please consider "
      "deleting the filter rules created by the wizard to get "
      "back to the former behavior.</p>"
      ) );
  mIntroText->setWordWrap(true);
  layout->addWidget( mIntroText );

  mScanProgressText = new QLabel( this );
  mScanProgressText->clear();
  mScanProgressText->setWordWrap( true );
  layout->addWidget( mScanProgressText );

  mToolsList = new QListWidget( this );
  mToolsList->hide();
  mToolsList->setSelectionMode( QAbstractItemView::MultiSelection );
  mToolsList->setLayoutMode( QListView::Batched );
  mToolsList->setBatchSize( 10 );
  layout->addWidget( mToolsList );
  connect( mToolsList->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
           this, SLOT(processSelectionChange(void)) );

  mSelectionHint = new QLabel( this );
  mSelectionHint->clear();
  layout->addWidget( mSelectionHint );

  layout->addStretch();
}


void ASWizInfoPage::setScanProgressText( const QString &toolName )
{
  mScanProgressText->setText( toolName );
}


void ASWizInfoPage::addAvailableTool( const QString &visibleName )
{
  mToolsList->addItem( visibleName );
  if ( !mToolsList->isVisible() )
  {
    mToolsList->show();
    mToolsList->selectionModel()->clearSelection();
    mToolsList->setCurrentRow( 0 );
    mSelectionHint->setText( i18n("<p>Please select the tools to be used "
                                  "for the detection and go "
                                  "to the next page.</p>") );
  }
}

bool ASWizInfoPage::isProgramSelected( const QString &visibleName )
{
  QString listName = visibleName;

  QList<QListWidgetItem*> foundItems = mToolsList->findItems( listName, Qt::MatchFixedString );
  return (foundItems.size() > 0 && foundItems[0]->isSelected());
}


void ASWizInfoPage::processSelectionChange()
{
  emit selectionChanged();
}


//---------------------------------------------------------------------------
ASWizSpamRulesPage::ASWizSpamRulesPage( QWidget * parent, const char * name)
  : ASWizPage( parent, name )
{
  QVBoxLayout *layout = new QVBoxLayout();
  mLayout->addItem( layout );

  mMarkRules = new QCheckBox( i18n("&Mark detected spam messages as read"), this );
  mMarkRules->setWhatsThis(
      i18n( "Mark messages which have been classified as spam as read.") );
  layout->addWidget( mMarkRules);

  mMoveSpamRules = new QCheckBox( i18n("Move &known spam to:"), this );
  mMoveSpamRules->setWhatsThis(
      i18n( "The default folder for spam messages is the trash folder, "
            "but you may change that in the folder view below.") );
  layout->addWidget( mMoveSpamRules );

  mFolderReqForSpamFolder = new FolderRequester( this );
  mFolderReqForSpamFolder->setFolder( KMKernel::self()->trashCollectionFolder() );
  mFolderReqForSpamFolder->setMustBeReadWrite( true );
  mFolderReqForSpamFolder->setShowOutbox( false );
  mFolderReqForSpamFolder->setShowImapFolders( false );

  QHBoxLayout *hLayout1 = new QHBoxLayout();
  layout->addItem( hLayout1 );
  hLayout1->addSpacing( KDialog::spacingHint() * 3 );
  hLayout1->addWidget( mFolderReqForSpamFolder );

  mMoveUnsureRules = new QCheckBox( i18n("Move &probable spam to:"), this );
  mMoveUnsureRules->setWhatsThis(
      i18n( "The default folder is the inbox folder, but you may change that "
            "in the folder view below.<p>"
            "Not all tools support a classification as unsure. If you have not "
            "selected a capable tool, you cannot select a folder as well.</p>") );
  layout->addWidget( mMoveUnsureRules );

  mFolderReqForUnsureFolder = new FolderRequester( this );
  mFolderReqForUnsureFolder->setFolder( "inbox" );
  mFolderReqForUnsureFolder->setMustBeReadWrite( true );
  mFolderReqForUnsureFolder->setShowOutbox( false );
  mFolderReqForUnsureFolder->setShowImapFolders( false );

  QHBoxLayout *hLayout2 = new QHBoxLayout();
  layout->addItem( hLayout2 );
  hLayout2->addSpacing( KDialog::spacingHint() * 3 );
  hLayout2->addWidget( mFolderReqForUnsureFolder );

  layout->addStretch();

  connect( mMarkRules, SIGNAL(clicked()),
            this, SLOT(processSelectionChange(void)) );
  connect( mMoveSpamRules, SIGNAL(clicked()),
            this, SLOT(processSelectionChange(void)) );
  connect( mMoveUnsureRules, SIGNAL(clicked()),
            this, SLOT(processSelectionChange(void)) );
  connect( mFolderReqForSpamFolder, SIGNAL(folderChanged(const Akonadi::Collection &)),
           this, SLOT(processSelectionChange(const Akonadi::Collection &)) );
  connect( mFolderReqForUnsureFolder, SIGNAL(folderChanged(const Akonadi::Collection &)),
           this, SLOT(processSelectionChange(const Akonadi::Collection&)) );

  mMarkRules->setChecked( true );
  mMoveSpamRules->setChecked( true );
}


bool ASWizSpamRulesPage::markAsReadSelected() const
{
  return mMarkRules->isChecked();
}


bool ASWizSpamRulesPage::moveSpamSelected() const
{
  return mMoveSpamRules->isChecked();
}


bool ASWizSpamRulesPage::moveUnsureSelected() const
{
  return mMoveUnsureRules->isChecked();
}

QString ASWizSpamRulesPage::selectedSpamCollectionId() const
{
  return QString::number( selectedSpamCollection().id() );
}

QString ASWizSpamRulesPage::selectedSpamCollectionName() const
{
  return selectedSpamCollection().name();
}

Akonadi::Collection ASWizSpamRulesPage::selectedSpamCollection() const
{
  if ( mFolderReqForSpamFolder->folderCollection().isValid() )
    return mFolderReqForSpamFolder->folderCollection();
  else
    return KMKernel::self()->trashCollectionFolder();
}


Akonadi::Collection ASWizSpamRulesPage::selectedUnsureCollection() const
{
  if ( mFolderReqForUnsureFolder->folderCollection().isValid() )
    return mFolderReqForUnsureFolder->folderCollection();
  else
    return KMKernel::self()->inboxCollectionFolder();
}

QString ASWizSpamRulesPage::selectedUnsureCollectionName() const
{
  return selectedUnsureCollection().name();
}

QString ASWizSpamRulesPage::selectedUnsureCollectionId() const
{
  return QString::number( selectedUnsureCollection().id() );
}


void ASWizSpamRulesPage::processSelectionChange()
{
  mFolderReqForSpamFolder->setEnabled( mMoveSpamRules->isChecked() );
  mFolderReqForUnsureFolder->setEnabled( mMoveUnsureRules->isChecked() );
  emit selectionChanged();
}


void ASWizSpamRulesPage::processSelectionChange( const Akonadi::Collection& )
{
  processSelectionChange();
}


void ASWizSpamRulesPage::allowUnsureFolderSelection( bool enabled )
{
  mMoveUnsureRules->setEnabled( enabled );
  mMoveUnsureRules->setVisible( enabled );
  mFolderReqForUnsureFolder->setEnabled( enabled );
  mFolderReqForUnsureFolder->setVisible( enabled );
}


//---------------------------------------------------------------------------
ASWizVirusRulesPage::ASWizVirusRulesPage( QWidget * parent, const char * name )
  : ASWizPage( parent, name )
{
  QGridLayout *grid = new QGridLayout();
  mLayout->addItem( grid );
  grid->setSpacing( KDialog::spacingHint() );

  mPipeRules = new QCheckBox( i18n("Check messages using the anti-virus tools"), this );
  mPipeRules->setWhatsThis(
      i18n( "Let the anti-virus tools check your messages. The wizard "
            "will create appropriate filters. The messages are usually "
            "marked by the tools so that following filters can react "
            "on this and, for example, move virus messages to a special folder.") );
  grid->addWidget( mPipeRules, 0, 0 );

  mMoveRules = new QCheckBox( i18n("Move detected viral messages to the selected folder"), this );
  mMoveRules->setWhatsThis(
      i18n( "A filter to detect messages classified as virus-infected and to move "
            "those messages into a predefined folder is created. The "
            "default folder is the trash folder, but you may change that "
            "in the folder view.") );
  grid->addWidget( mMoveRules, 1, 0 );

  mMarkRules = new QCheckBox( i18n("Additionally, mark detected viral messages as read"), this );
  mMarkRules->setEnabled( false );
  mMarkRules->setWhatsThis(
      i18n( "Mark messages which have been classified as "
            "virus-infected as read, as well as moving them "
            "to the selected folder.") );
  grid->addWidget( mMarkRules, 2, 0 );
  mFolderTree = new FolderTreeWidget( this, 0, FolderTreeWidget::None );
  mFolderTree->readableCollectionProxyModel()->setAccessRights( Akonadi::Collection::CanCreateCollection );
  mFolderTree->selectCollectionFolder( KMKernel::self()->trashCollectionFolder() );
  mFolderTree->folderTreeView()->setDragDropMode( QAbstractItemView::NoDragDrop );

  mFolderTree->disableContextMenuAndExtraColumn();
  grid->addWidget( mFolderTree, 3, 0 );

  connect( mPipeRules, SIGNAL(clicked()),
            this, SLOT(processSelectionChange(void)) );
  connect( mMoveRules, SIGNAL(clicked()),
            this, SLOT(processSelectionChange(void)) );
  connect( mMarkRules, SIGNAL(clicked()),
            this, SLOT(processSelectionChange(void)) );
  connect( mMoveRules, SIGNAL( toggled( bool ) ),
           mMarkRules, SLOT( setEnabled( bool ) ) );

}

bool ASWizVirusRulesPage::pipeRulesSelected() const
{
  return mPipeRules->isChecked();
}


bool ASWizVirusRulesPage::moveRulesSelected() const
{
  return mMoveRules->isChecked();
}

bool ASWizVirusRulesPage::markReadRulesSelected() const
{
  return mMarkRules->isChecked();
}


QString ASWizVirusRulesPage::selectedFolderName() const
{
  if ( mFolderTree->selectedCollection().isValid() )
    return QString::number( mFolderTree->selectedCollection().id() );
  else
    return QString::number( KMKernel::self()->trashCollectionFolder().id() );
}

void ASWizVirusRulesPage::processSelectionChange()
{
  emit selectionChanged();
}


//---------------------------------------------------------------------------
ASWizSummaryPage::ASWizSummaryPage( QWidget * parent, const char * name )
  : ASWizPage( parent, name )
{
  QBoxLayout * layout = new QVBoxLayout();
  mLayout->addItem( layout );

  mSummaryText = new QLabel( this );
  layout->addWidget( mSummaryText );
  layout->addStretch();
}


void ASWizSummaryPage::setSummaryText( const QString & text )
{
  mSummaryText->setText( text );
}


#include "antispamwizard.moc"
