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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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
#include "kcursorsaver.h"
#include "kmfilter.h"
#include "kmfilteraction.h"
#include "kmfiltermgr.h"
#include "kmkernel.h"
#include "kmfolderseldlg.h"
#include "kmfoldertree.h"
#include "kmmainwin.h"

#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kdialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprocess.h>

#include <qdom.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

using namespace KMail;

AntiSpamWizard::AntiSpamWizard( QWidget* parent, KMFolderTree * mainFolderTree,
                                KActionCollection * collection )
  : KWizard( parent )
{
  // read the configuration for the anti-spam tools
  ConfigReader reader( mToolList );
  reader.readAndMergeConfig();
  mToolList = reader.getToolList();

#ifndef NDEBUG
    kdDebug(5006) << endl << "Considered anti-spam/virus tools: " << endl;
#endif
  QStringList descriptionList;
  QStringList whatsThisList;
  QValueListIterator<SpamToolConfig> it = mToolList.begin();
  while ( it != mToolList.end() )
  {
    descriptionList.append( (*it).getVisibleName() );
    whatsThisList.append( (*it).getWhatsThisText() );
#ifndef NDEBUG
    kdDebug(5006) << "Predefined tool: " << (*it).getId() << endl;
    kdDebug(5006) << "Config version: " << (*it).getVersion() << endl;
    kdDebug(5006) << "Displayed name: " << (*it).getVisibleName() << endl;
    kdDebug(5006) << "Executable: " << (*it).getExecutable() << endl;
    kdDebug(5006) << "WhatsThis URL: " << (*it).getWhatsThisText() << endl;
    kdDebug(5006) << "Filter name: " << (*it).getFilterName() << endl;
    kdDebug(5006) << "Detection command: " << (*it).getDetectCmd() << endl;
    kdDebug(5006) << "Learn spam command: " << (*it).getSpamCmd() << endl;
    kdDebug(5006) << "Learn ham command: " << (*it).getHamCmd() << endl;
    kdDebug(5006) << "Detection header: " << (*it).getDetectionHeader() << endl;
    kdDebug(5006) << "Detection pattern: " << (*it).getDetectionPattern() << endl;
    kdDebug(5006) << "Use as RegExp: " << (*it).isUseRegExp() << endl;
    kdDebug(5006) << "Supports Bayes Filter: " << (*it).useBayesFilter() << endl;
    kdDebug(5006) << "Type: " << (*it).getType() << endl << endl;
#endif
    it++;
  }

  mActionCollection = collection;

  setCaption( i18n( "Anti-Spam/Virus Wizard" ));
  mInfoPage = new ASWizInfoPage( 0, "" );
  addPage( mInfoPage, i18n( "Welcome to the KMail Anti-Spam/Virus Wizard." ));
  mProgramsPage = new ASWizProgramsPage( 0, "", descriptionList, whatsThisList );
  addPage( mProgramsPage, i18n( "Please select the tools to be used by KMail." ));
  mSpamRulesPage = new ASWizSpamRulesPage( 0, "", mainFolderTree );
  mVirusRulesPage = new ASWizVirusRulesPage( 0, "", mainFolderTree );

  connect( mProgramsPage, SIGNAL( selectionChanged( void ) ),
            this, SLOT( checkProgramsSelections( void ) ) );
  connect( mSpamRulesPage, SIGNAL( selectionChanged( void ) ),
            this, SLOT( checkSpamRulesSelections( void ) ) );
  connect( mVirusRulesPage, SIGNAL( selectionChanged( void ) ),
            this, SLOT( checkVirusRulesSelections( void ) ) );

  connect( this, SIGNAL( helpClicked( void) ),
            this, SLOT( slotHelpClicked( void ) ) );

  setNextEnabled( mInfoPage, false );
  setNextEnabled( mProgramsPage, false );

  QTimer::singleShot( 500, this, SLOT( checkToolAvailability( void ) ) );
}


void AntiSpamWizard::accept()
{
  kdDebug( 5006 ) << "Folder name for spam is "
                  << mSpamRulesPage->selectedFolderName() << endl;
  kdDebug( 5006 ) << "Folder name for viruses is "
                  << mVirusRulesPage->selectedFolderName() << endl;

  KMFilterActionDict dict;
  
  // Let's start with virus detection and handling,
  // so we can avoid spam checks for viral messages
  QValueListIterator<SpamToolConfig> it = mToolList.begin();
  while ( it != mToolList.end() )
  {
    if ( mProgramsPage->isProgramSelected( (*it).getVisibleName() ) &&
       ( mVirusRulesPage->pipeRulesSelected() && (*it).isVirusTool() ) )
    {
      // pipe messages through the anti-virus tools,
      // one single filter for each tool
      // (could get combined but so it's easier to understand for the user)
      KMFilter* pipeFilter = new KMFilter();
      QPtrList<KMFilterAction>* pipeFilterActions = pipeFilter->actions();
      KMFilterAction* pipeFilterAction = dict["filter app"]->create();
      pipeFilterAction->argsFromString( (*it).getDetectCmd() );
      pipeFilterActions->append( pipeFilterAction );
      KMSearchPattern* pipeFilterPattern = pipeFilter->pattern();
      pipeFilterPattern->setName( (*it).getFilterName() );
      pipeFilterPattern->append( KMSearchRule::createInstance( "<size>",
                                 KMSearchRule::FuncIsGreaterOrEqual, "0" ) );
      pipeFilter->setApplyOnOutbound( FALSE);
      pipeFilter->setApplyOnInbound();
      pipeFilter->setApplyOnExplicit();
      pipeFilter->setStopProcessingHere( FALSE );
      pipeFilter->setConfigureShortcut( FALSE );

      KMKernel::self()->filterMgr()->appendFilter( pipeFilter );
    }
    it++;
  }

  if ( mVirusRulesPage->moveRulesSelected() )
  {
    // Sort out viruses depending on header fields set by the tools
    KMFilter* virusFilter = new KMFilter();
    QPtrList<KMFilterAction>* virusFilterActions = virusFilter->actions();
    KMFilterAction* virusFilterAction1 = dict["transfer"]->create();
    virusFilterAction1->argsFromString( mVirusRulesPage->selectedFolderName() );
    virusFilterActions->append( virusFilterAction1 );
    if ( mVirusRulesPage->markReadRulesSelected() ) {
      KMFilterAction* virusFilterAction2 = dict["set status"]->create();
      virusFilterAction2->argsFromString( "R" ); // Read
      virusFilterActions->append( virusFilterAction2 );
    }   
    KMSearchPattern* virusFilterPattern = virusFilter->pattern();
    virusFilterPattern->setName( i18n( "Virus handling" ) );
    virusFilterPattern->setOp( KMSearchPattern::OpOr );
    it = mToolList.begin();
    while ( it != mToolList.end() )
    {
      if ( mProgramsPage->isProgramSelected( (*it).getVisibleName() ))
      {
        if ( (*it).isVirusTool() )
        {
            const QCString header = (*it).getDetectionHeader().ascii();
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
      it++;
    }
    virusFilter->setApplyOnOutbound( FALSE);
    virusFilter->setApplyOnInbound();
    virusFilter->setApplyOnExplicit();
    virusFilter->setStopProcessingHere( TRUE );
    virusFilter->setConfigureShortcut( FALSE );

    KMKernel::self()->filterMgr()->appendFilter( virusFilter );
  }

  it = mToolList.begin();
  while ( it != mToolList.end() )
  {
    if ( mProgramsPage->isProgramSelected( (*it).getVisibleName() ) &&
       ( mSpamRulesPage->pipeRulesSelected() && (*it).isSpamTool() ) )
    {
      // pipe messages through the anti-spam tools,
      // one single filter for each tool
      // (could get combined but so it's easier to understand for the user)
      KMFilter* pipeFilter = new KMFilter();
      QPtrList<KMFilterAction>* pipeFilterActions = pipeFilter->actions();
      KMFilterAction* pipeFilterAction = dict["filter app"]->create();
      pipeFilterAction->argsFromString( (*it).getDetectCmd() );
      pipeFilterActions->append( pipeFilterAction );
      KMSearchPattern* pipeFilterPattern = pipeFilter->pattern();
      pipeFilterPattern->setName( (*it).getFilterName() );
      pipeFilterPattern->append( KMSearchRule::createInstance( "<size>",
                                 KMSearchRule::FuncIsGreaterOrEqual, "0" ) );
      pipeFilter->setApplyOnOutbound( FALSE);
      pipeFilter->setApplyOnInbound();
      pipeFilter->setApplyOnExplicit();
      pipeFilter->setStopProcessingHere( FALSE );
      pipeFilter->setConfigureShortcut( FALSE );

      KMKernel::self()->filterMgr()->appendFilter( pipeFilter );
    }
    it++;
  }

  if ( mSpamRulesPage->moveRulesSelected() )
  {
    // Sort out spam depending on header fields set by the tools
    KMFilter* spamFilter = new KMFilter();
    QPtrList<KMFilterAction>* spamFilterActions = spamFilter->actions();
    KMFilterAction* spamFilterAction1 = dict["transfer"]->create();
    spamFilterAction1->argsFromString( mSpamRulesPage->selectedFolderName() );
    spamFilterActions->append( spamFilterAction1 );
    KMFilterAction* spamFilterAction2 = dict["set status"]->create();
    spamFilterAction2->argsFromString( "P" ); // Spam
    spamFilterActions->append( spamFilterAction2 );
    if ( mSpamRulesPage->markReadRulesSelected() ) {
      KMFilterAction* spamFilterAction3 = dict["set status"]->create();
      spamFilterAction3->argsFromString( "R" ); // Read
      spamFilterActions->append( spamFilterAction3 );
    }
    KMSearchPattern* spamFilterPattern = spamFilter->pattern();
    spamFilterPattern->setName( i18n( "Spam handling" ) );
    spamFilterPattern->setOp( KMSearchPattern::OpOr );
    it = mToolList.begin();
    while ( it != mToolList.end() )
    {
      if ( mProgramsPage->isProgramSelected( (*it).getVisibleName() ) )
      {
          if ( (*it).isSpamTool() )
          {
            const QCString header = (*it).getDetectionHeader().ascii();
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
      it++;
    }
    spamFilter->setApplyOnOutbound( FALSE);
    spamFilter->setApplyOnInbound();
    spamFilter->setApplyOnExplicit();
    spamFilter->setStopProcessingHere( TRUE );
    spamFilter->setConfigureShortcut( FALSE );

    KMKernel::self()->filterMgr()->appendFilter( spamFilter );
  }

  if ( mSpamRulesPage->classifyRulesSelected() )
  {
    // Classify messages manually as Spam
    KMFilter* classSpamFilter = new KMFilter();
    classSpamFilter->setIcon( "mark_as_spam" );
    QPtrList<KMFilterAction>* classSpamFilterActions = classSpamFilter->actions();
    KMFilterAction* classSpamFilterActionFirst = dict["set status"]->create();
    classSpamFilterActionFirst->argsFromString( "P" );
    classSpamFilterActions->append( classSpamFilterActionFirst );
    it = mToolList.begin();
    while ( it != mToolList.end() )
    {
      if ( mProgramsPage->isProgramSelected( (*it).getVisibleName() )
          && (*it).useBayesFilter() )
      {
        KMFilterAction* classSpamFilterAction = dict["execute"]->create();
        classSpamFilterAction->argsFromString( (*it).getSpamCmd() );
        classSpamFilterActions->append( classSpamFilterAction );
      }
      it++;
    }
    KMFilterAction* classSpamFilterActionLast = dict["transfer"]->create();
    classSpamFilterActionLast->argsFromString( mSpamRulesPage->selectedFolderName() );
    classSpamFilterActions->append( classSpamFilterActionLast );

    KMSearchPattern* classSpamFilterPattern = classSpamFilter->pattern();
    classSpamFilterPattern->setName( i18n( "Classify as spam" ) );
    classSpamFilterPattern->append( KMSearchRule::createInstance( "<size>",
                                    KMSearchRule::FuncIsGreaterOrEqual, "0" ) );
    classSpamFilter->setApplyOnOutbound( FALSE);
    classSpamFilter->setApplyOnInbound( FALSE );
    classSpamFilter->setApplyOnExplicit( FALSE );
    classSpamFilter->setStopProcessingHere( TRUE );
    classSpamFilter->setConfigureShortcut( TRUE );
    KMKernel::self()->filterMgr()->appendFilter( classSpamFilter );

    // Classify messages manually as not Spam / as Ham
    KMFilter* classHamFilter = new KMFilter();
    QPtrList<KMFilterAction>* classHamFilterActions = classHamFilter->actions();
    KMFilterAction* classHamFilterActionFirst = dict["set status"]->create();
    classHamFilterActionFirst->argsFromString( "H" );
    classHamFilterActions->append( classHamFilterActionFirst );
    it = mToolList.begin();
    while ( it != mToolList.end() )
    {
      if ( mProgramsPage->isProgramSelected( (*it).getVisibleName() )
          && (*it).useBayesFilter() )
      {
        KMFilterAction* classHamFilterAction = dict["execute"]->create();
        classHamFilterAction->argsFromString( (*it).getHamCmd() );
        classHamFilterActions->append( classHamFilterAction );
      }
      it++;
    }
    KMSearchPattern* classHamFilterPattern = classHamFilter->pattern();
    classHamFilterPattern->setName( i18n( "Classify as NOT spam" ) );
    classHamFilterPattern->append( KMSearchRule::createInstance( "<size>",
                                   KMSearchRule::FuncIsGreaterOrEqual, "0" ) );
    classHamFilter->setApplyOnOutbound( FALSE);
    classHamFilter->setApplyOnInbound( FALSE );
    classHamFilter->setApplyOnExplicit( FALSE );
    classHamFilter->setStopProcessingHere( TRUE );
    classHamFilter->setConfigureShortcut( TRUE );
    KMKernel::self()->filterMgr()->appendFilter( classHamFilter );

    // add the classification filter actions to the toolbar
    QString filterNameSpam =
        QString( "Filter %1" ).arg( classSpamFilterPattern->name() );
    filterNameSpam = filterNameSpam.replace( " ", "_" );
    QString filterNameHam =
        QString( "Filter %1" ).arg( classHamFilterPattern->name() );
    filterNameHam = filterNameHam.replace( " ", "_" );

    // FIXME post KDE 3.2
    // The following code manipulates the kmmainwin.rc file directly. Usuallay
    // one would expect to let the toolbar write back it's change from above
    // i.e. the new structure including the two added actions.
    // In KDE 3.2 there is no API for that so I only fund the way to read in
    // the XML file myself, to change it and write it out then.
    // As soon as an API is available, the following part can certainly get
    // replaced by one or two statements.
    // (a.gungl@gmx.de)

    // make the toolbar changes persistent - let's be very conservative here
    QString config =
        KXMLGUIFactory::readConfigFile( "kmmainwin.rc", KMKernel::self()->xmlGuiInstance() );
#ifndef NDEBUG
    kdDebug(5006) << "Read kmmainwin.rc contents (last 1000 chars printed):" << endl;
    kdDebug(5006) << config.right( 1000 ) << endl;
    kdDebug(5006) << "#####################################################" << endl;
#endif
    QDomDocument domDoc;
    domDoc.setContent( config );
    QDomNodeList domNodeList = domDoc.elementsByTagName( "ToolBar" );
    if ( domNodeList.count() > 0 )
      kdDebug(5006) << "ToolBar section found." << endl;
    else
      kdDebug(5006) << "No ToolBar section found." << endl;
    for ( unsigned int i = 0; i < domNodeList.count(); i++ )
    {
      QDomNode domNode = domNodeList.item( i );
      QDomNamedNodeMap nodeMap = domNode.attributes();
      kdDebug(5006) << "name=" << nodeMap.namedItem( "name" ).nodeValue() << endl;
      if ( nodeMap.namedItem( "name" ).nodeValue() == "mainToolBar" )
      {
        kdDebug(5006) << "mainToolBar section found." << endl;
        bool spamActionFound = false;
        bool hamActionFound = false;
        QDomNodeList domNodeChildList = domNode.childNodes();
        for ( unsigned int j = 0; j < domNodeChildList.count(); j++ )
        {
          QDomNode innerDomNode = domNodeChildList.item( j );
          QDomNamedNodeMap innerNodeMap = innerDomNode.attributes();
          if ( innerNodeMap.namedItem( "name" ).nodeValue() == filterNameSpam )
            spamActionFound = true;
          if ( innerNodeMap.namedItem( "name" ).nodeValue() == filterNameHam )
            hamActionFound = true;
        }

        // append the new actions if not yet existing
        if ( !spamActionFound )
        {
          QDomElement domElemSpam = domDoc.createElement( "Action" );
          domElemSpam.setAttribute( "name", filterNameSpam );
          domNode.appendChild( domElemSpam );
          kdDebug(5006) << "Spam action added to toolbar." << endl;
        }
        if ( !hamActionFound )
        {
          QDomElement domElemHam = domDoc.createElement( "Action" );
          domElemHam.setAttribute( "name", filterNameHam );
          domNode.appendChild( domElemHam );
          kdDebug(5006) << "Ham action added to toolbar." << endl;
        }
        if ( !spamActionFound || !hamActionFound )
        {
#ifndef NDEBUG
          kdDebug(5006) << "New kmmainwin.rc structur (last 1000 chars printed):" << endl;
          kdDebug(5006) << domDoc.toString().right( 1000 ) << endl;
          kdDebug(5006) << "####################################################" << endl;
#endif
          // write back the modified resource file
          KXMLGUIFactory::saveConfigFile( domDoc, "kmmainwin.rc",
              KMKernel::self()->xmlGuiInstance() );
        }
      }
      else
        kdDebug(5006) << "No mainToolBar section found." << endl;
    }
  }

  QDialog::accept();
}


void AntiSpamWizard::checkProgramsSelections()
{
  bool status = false;
  bool canClassify = false;
  QValueListIterator<SpamToolConfig> it = mToolList.begin();
  mSpamToolsUsed = false;
  mVirusToolsUsed = false;
  while ( it != mToolList.end() )
  {
    if ( mProgramsPage->isProgramSelected( (*it).getVisibleName() ) )
    {
      status = true;
      if ( (*it).isSpamTool() )
        mSpamToolsUsed = true;
      if ( (*it).isVirusTool() )
        mVirusToolsUsed = true;
    }
    if ( (*it).useBayesFilter() )
      canClassify = true;
    it++;
  }

  mSpamRulesPage->allowClassification( canClassify );
  
  removePage( mSpamRulesPage );
  removePage( mVirusRulesPage );
  if ( mSpamToolsUsed )
  {
    addPage( mSpamRulesPage, i18n( "Please select the spam filters to be created inside KMail." ));
    checkSpamRulesSelections();
  }
  if ( mVirusToolsUsed )
  {
    addPage( mVirusRulesPage, i18n( "Please select the virus filters to be created inside KMail." ));
    checkVirusRulesSelections();
  }

  setNextEnabled( mProgramsPage, status );
}


void AntiSpamWizard::checkSpamRulesSelections()
{
  if ( anySpamOptionChecked() )
  {
    if ( mVirusToolsUsed )
      setNextEnabled( mSpamRulesPage, true );
    else
      setFinishEnabled( mSpamRulesPage, true );
  }
  else
  {
    setNextEnabled( mSpamRulesPage, mVirusToolsUsed );
    setFinishEnabled( mSpamRulesPage, false );
  }
  if ( mVirusToolsUsed )
    checkVirusRulesSelections();
}

void AntiSpamWizard::checkVirusRulesSelections()
{
  if ( anyVirusOptionChecked() )
    setFinishEnabled( mVirusRulesPage, true );
  else
    setFinishEnabled( mVirusRulesPage, 
                      anySpamOptionChecked() && mSpamToolsUsed );
}


void AntiSpamWizard::checkToolAvailability()
{
  KCursorSaver busy(KBusyPtr::busy()); // this can take some time to find the tools

  // checkboxes for the tools
  QValueListIterator<SpamToolConfig> it = mToolList.begin();
  while ( it != mToolList.end() )
  {
    QString text( i18n("Scanning for %1...").arg( (*it).getId() ) );
    mInfoPage->setScanProgressText( text );
    KApplication::kApplication()->processEvents( 200 );
    int rc = checkForProgram( (*it).getExecutable() );
    mProgramsPage->setProgramAsFound( (*it).getVisibleName(), !rc );
    it++;
  }
  mInfoPage->setScanProgressText( i18n("Scanning for anti-spam/virus tools finished.") );
  setNextEnabled( mInfoPage, true );
}


int AntiSpamWizard::checkForProgram( QString executable )
{
  kdDebug(5006) << "Testing for executable:" << executable << endl;
  KProcess process;
  process << executable;
  process.setUseShell( true );
  process.start( KProcess::Block );
  return process.exitStatus();
}


void AntiSpamWizard::slotHelpClicked()
{
  KApplication::kApplication()->invokeHelp( "the-anti-spam-wizard", "kmail" );
}


bool AntiSpamWizard::anySpamOptionChecked()
{
  return ( mSpamRulesPage->moveRulesSelected()
        || mSpamRulesPage->pipeRulesSelected()
        || mSpamRulesPage->classifyRulesSelected() );
}

bool AntiSpamWizard::anyVirusOptionChecked()
{
  return ( mVirusRulesPage->moveRulesSelected() || mVirusRulesPage->pipeRulesSelected() );
}


//---------------------------------------------------------------------------
AntiSpamWizard::SpamToolConfig::SpamToolConfig(QString toolId,
      int configVersion,QString name, QString exec,
      QString url, QString filter, QString detection, QString spam, QString ham,
      QString header, QString pattern, bool regExp, bool bayesFilter, QString type)
  : mId( toolId ), mVersion( configVersion ),
    mVisibleName( name ), mExecutable( exec ), mWhatsThisText( url ),
    mFilterName( filter ), mDetectCmd( detection ), mSpamCmd( spam ),
    mHamCmd( ham ), mDetectionHeader( header ), mDetectionPattern( pattern ),
    mUseRegExp( regExp ), mSupportsBayesFilter( bayesFilter ), mType( type )
{
}


//---------------------------------------------------------------------------
AntiSpamWizard::ConfigReader::ConfigReader( QValueList<SpamToolConfig> & configList )
  : mToolList( configList ),
    mConfig( "kmail.antispamrc", true )
{}


void AntiSpamWizard::ConfigReader::readAndMergeConfig()
{
  // read the configuration from the global config file
  mConfig.setReadDefaults( true );
  KConfigGroup general( &mConfig, "General" );
  int registeredTools = general.readNumEntry( "tools", 0 );
  for (int i = 1; i <= registeredTools; i++)
  {
    KConfigGroup toolConfig( &mConfig,
      QCString("Spamtool #") + QCString().setNum(i) );
    mToolList.append( readToolConfig( toolConfig ) );
  }

  // read the configuration from the user config file
  // and merge newer config data
  mConfig.setReadDefaults( false );
  KConfigGroup user_general( &mConfig, "General" );
  int user_registeredTools = user_general.readNumEntry( "tools", 0 );
  for (int i = 1; i <= user_registeredTools; i++)
  {
    KConfigGroup toolConfig( &mConfig,
      QCString("Spamtool #") + QCString().setNum(i) );
    mergeToolConfig( readToolConfig( toolConfig ) );
  }
  // Make sure to have add least one tool listed even when the
  // config file was not found or whatever went wrong
  if ( registeredTools < 1 && user_registeredTools < 1)
    mToolList.append( createDummyConfig() );
}


AntiSpamWizard::SpamToolConfig
    AntiSpamWizard::ConfigReader::readToolConfig( KConfigGroup & configGroup )
{
  QString id = configGroup.readEntry( "Ident" );
  int version = configGroup.readNumEntry( "Version" );
#ifndef NDEBUG
  kdDebug(5006) << "Found predefined tool: " << id << endl;
  kdDebug(5006) << "With config version  : " << version << endl;
#endif
  QString name = configGroup.readEntry( "VisibleName" );
  QString executable = configGroup.readEntry( "Executable" );
  QString url = configGroup.readEntry( "URL" );
  QString filterName = configGroup.readEntry( "PipeFilterName" );
  QString detectCmd = configGroup.readEntry( "PipeCmdDetect" );
  QString spamCmd = configGroup.readEntry( "ExecCmdSpam" );
  QString hamCmd = configGroup.readEntry( "ExecCmdHam" );
  QString header = configGroup.readEntry( "DetectionHeader" );
  QString pattern = configGroup.readEntry( "DetectionPattern" );
  bool useRegExp  = configGroup.readBoolEntry( "UseRegExp" );
  bool supportsBayes = configGroup.readBoolEntry( "SupportsBayes" );
  QString type = configGroup.readEntry( "Type" );
  return SpamToolConfig( id, version, name, executable, url,
                         filterName, detectCmd, spamCmd, hamCmd,
                         header, pattern, useRegExp, supportsBayes, type );
}


AntiSpamWizard::SpamToolConfig AntiSpamWizard::ConfigReader::createDummyConfig()
{
  return SpamToolConfig( "spamassassin", 0,
                        "&SpamAssassin", "spamassassin -V",
                        "http://spamassassin.org", "SpamAssassin Check",
                        "spamassassin -L",
                        "sa-learn -L --spam --no-rebuild --single",
                        "sa-learn -L --ham --no-rebuild --single",
                        "X-Spam-Flag", "yes",
                        false, true, "spam" );
}


void AntiSpamWizard::ConfigReader::mergeToolConfig( AntiSpamWizard::SpamToolConfig config )
{
  bool found = false;
  QValueListIterator<SpamToolConfig> it = mToolList.begin();
  while ( it != mToolList.end() && !found)
  {
#ifndef NDEBUG
    kdDebug(5006) << "Check against tool: " << (*it).getId() << endl;
    kdDebug(5006) << "Against version   : " << (*it).getVersion() << endl;
#endif
    if ( (*it).getId() == config.getId() )
    {
      found = true;
      if ( (*it).getVersion() < config.getVersion() )
      {
#ifndef NDEBUG
        kdDebug(5006) << "Replacing config ..." << endl;
#endif
        mToolList.remove( it );
        mToolList.append( config );
      }
    }
    else it++;
  }
  if ( !found )
    mToolList.append( config );
}


//---------------------------------------------------------------------------
ASWizInfoPage::ASWizInfoPage( QWidget * parent, const char * name )
  : QWidget( parent, name )
{
  QGridLayout *grid = new QGridLayout( this, 1, 1, KDialog::marginHint(),
                                        KDialog::spacingHint() );
  grid->setColStretch( 1, 10 );

  mIntroText = new QLabel( this );
  mIntroText->setText( i18n(
    "<p>Here you get some assistance in setting up KMail's filter "
    "rules to use some commonly-known anti-spam/virus tools.</p>"
    "<p>The wizard can detect those tools on your computer as "
    "well as create filter rules to classify messages using these "
    "tools and to separate messages classified as spam, or which "
    "contain viruses. The wizard will not take any existing filter "
    "rules into consideration but will append new rules in any"
    "case.</p>"
    "<p><b>WARNING:</b> As KMail is blocked during the scan of the "
    "messages for spam or viruses, you may encounter problems with "
    "the responsivness of KMail because anti-spam/virus tool "
    "operations are usually time consuming. Please consider "
    "deleting the filter rules created by the wizard to get "
    "back to the former behaviour."
    ) );
  grid->addWidget( mIntroText, 0, 0 );

  mScanProgressText = new QLabel( this );
  mScanProgressText->setText( "" ) ;
  grid->addWidget( mScanProgressText, 1, 0 );
}

void ASWizInfoPage::setScanProgressText( const QString &toolName )
{
  mScanProgressText->setText( toolName );
}

//---------------------------------------------------------------------------
ASWizProgramsPage::ASWizProgramsPage( QWidget * parent, const char * name,
                                      QStringList &checkBoxTextList,
                                      QStringList &checkBoxWhatsThisList )
  : QWidget( parent, name )
{
  QGridLayout *grid = new QGridLayout( this, 3, 1, KDialog::marginHint(),
                                        KDialog::spacingHint() );
  // checkboxes for the tools
  int row = 0;
  QStringList::Iterator it1 = checkBoxTextList.begin();
  QStringList::Iterator it2 = checkBoxWhatsThisList.begin();
  while ( it1 != checkBoxTextList.end() )
  {
    QCheckBox *box = new QCheckBox( *it1, this );
    if ( it2 != checkBoxWhatsThisList.end() )
    {
      QWhatsThis::add( box, *it2 );
      QToolTip::add( box, *it2 );
      ++it2;
    }
    grid->addWidget( box, row++, 0 );
    connect( box, SIGNAL(clicked()),
             this, SLOT(processSelectionChange(void)) );
    mProgramDict.insert( *it1, box );
    ++it1;
  }

  // hint text
  QLabel *introText = new QLabel( this );
  introText->setText( i18n(
    "<p>For these tools it is possible to let the "
    "wizard create filter rules. KMail tried to find the tools "
    "in the PATH of your system; the wizard does not allow you "
    "to create rules for tools which were not found: "
    "this is to keep your configuration consistent and "
    "to minimize the risk of unpredicted behavior.</p>"
    ) );
  grid->addWidget( introText, row++, 0 );
}


bool ASWizProgramsPage::isProgramSelected( const QString &visibleName )
{
  if ( mProgramDict[visibleName] )
    return mProgramDict[visibleName]->isChecked();
  else
    return false;
}


void ASWizProgramsPage::setProgramAsFound( const QString &visibleName, bool found )
{
  QCheckBox * box = mProgramDict[visibleName];
  if ( box )
  {
    QString foundText( i18n("(found on this system)") );
    QString notFoundText( i18n("(not found on this system)") );
    QString labelText = visibleName;
    labelText += " ";
    if ( found )
      labelText += foundText;
    else
    {
      labelText += notFoundText;
      box->setEnabled( false );
    }
    box->setText( labelText );
  }
}


void ASWizProgramsPage::processSelectionChange()
{
  emit selectionChanged();
}

//---------------------------------------------------------------------------
ASWizSpamRulesPage::ASWizSpamRulesPage( QWidget * parent, const char * name,
                                        KMFolderTree * mainFolderTree )
  : QWidget( parent, name )
{
  QGridLayout *grid = new QGridLayout( this, 5, 1, KDialog::marginHint(),
                                       KDialog::spacingHint() );

  mClassifyRules = new QCheckBox( i18n("Classify messages manually as spam"), this );
  QWhatsThis::add( mClassifyRules,
      i18n( "Sometimes messages are classified wrongly or even not at all; "
            "the latter might be by intention, because you perhaps filter "
            "out messages from mailing lists before you let the anti-spam "
            "tools classify the rest of the messages. You can correct these "
            "wrong or missing classifications manually by using the "
            "appropriate toolbar buttons which trigger special filters "
            "created by this wizard." ) );
  grid->addWidget( mClassifyRules, 0, 0 );

  mPipeRules = new QCheckBox( i18n("Classify messages using the anti-spam tools"), this );
  QWhatsThis::add( mPipeRules,
      i18n( "Let the anti-spam tools classify your messages. The wizard "
            "will create appropriate filters. The messages are usually "
            "marked by the tools so that following filters can react "
            "on this and, for example, move spam messages to a special folder.") );
  grid->addWidget( mPipeRules, 1, 0 );

  mMoveRules = new QCheckBox( i18n("Move detected spam messages to the selected folder"), this );
  QWhatsThis::add( mMoveRules,
      i18n( "A filter to detect messages classified as spam and to move "
            "those messages into a predefined folder is created. The "
            "default folder is the trash folder, but you may change that "
            "in the folder view.") );
  grid->addWidget( mMoveRules, 2, 0 );
  
  mMarkRules = new QCheckBox( i18n("Additionally, mark detected spam messages as read"), this );
  mMarkRules->setEnabled( false );
  QWhatsThis::add( mMarkRules,
      i18n( "Mark messages which have been classified as "
            "spam as read, as well as moving them to the selected "
            "folder.") );
  grid->addWidget( mMarkRules, 3, 0 );
  
  QString s = "trash";
  mFolderTree = new SimpleFolderTree( this, mainFolderTree, s );
  grid->addWidget( mFolderTree, 4, 0 );

  connect( mPipeRules, SIGNAL(clicked()),
            this, SLOT(processSelectionChange(void)) );
  connect( mClassifyRules, SIGNAL(clicked()),
            this, SLOT(processSelectionChange(void)) );
  connect( mMoveRules, SIGNAL(clicked()),
            this, SLOT(processSelectionChange(void)) );
  connect( mMarkRules, SIGNAL(clicked()),
            this, SLOT(processSelectionChange(void)) );
  connect( mMoveRules, SIGNAL( toggled( bool ) ),
           mMarkRules, SLOT( setEnabled( bool ) ) );
}

bool ASWizSpamRulesPage::pipeRulesSelected() const
{
  return mPipeRules->isChecked();
}


bool ASWizSpamRulesPage::classifyRulesSelected() const
{
  return mClassifyRules->isChecked();
}


bool ASWizSpamRulesPage::moveRulesSelected() const
{
  return mMoveRules->isChecked();
}

bool ASWizSpamRulesPage::markReadRulesSelected() const
{
  return mMarkRules->isChecked();
}


QString ASWizSpamRulesPage::selectedFolderName() const
{
  QString name = "trash";
  if ( mFolderTree->folder() )
    name = mFolderTree->folder()->idString();
  return name;
}

void ASWizSpamRulesPage::processSelectionChange()
{
  emit selectionChanged();
}


void ASWizSpamRulesPage::allowClassification( bool enabled )
{
  if ( enabled )
    mClassifyRules->setEnabled( true );
  else
  {
    mClassifyRules->setChecked( false );
    mClassifyRules->setEnabled( false );
  }
}

//---------------------------------------------------------------------------
ASWizVirusRulesPage::ASWizVirusRulesPage( QWidget * parent, const char * name,
                                  KMFolderTree * mainFolderTree )
  : QWidget( parent, name )
{
  QGridLayout *grid = new QGridLayout( this, 5, 1, KDialog::marginHint(),
                                       KDialog::spacingHint() );

  mPipeRules = new QCheckBox( i18n("Classify messages using the anti-virus tools"), this );
  QWhatsThis::add( mPipeRules,
      i18n( "Let the anti-virus tools classify your messages. The wizard "
            "will create appropriate filters. The messages are usually "
            "marked by the tools so that following filters can react "
            "on this and, for example, move virus messages to a special folder.") );
  grid->addWidget( mPipeRules, 0, 0 );

  mMoveRules = new QCheckBox( i18n("Move detected viral messages to the selected folder"), this );
  QWhatsThis::add( mMoveRules,
      i18n( "A filter to detect messages classified as virus-infected and to move "
            "those messages into a predefined folder is created. The "
            "default folder is the trash folder, but you may change that "
            "in the folder view.") );
  grid->addWidget( mMoveRules, 1, 0 );
  
  mMarkRules = new QCheckBox( i18n("Additionally, mark detected viral messages as read"), this );
  mMarkRules->setEnabled( false );
  QWhatsThis::add( mMarkRules,
      i18n( "Mark messages which have been classified as "
            "virus-infected as read, as well as moving them "
            "to the selected folder.") );
  grid->addWidget( mMarkRules, 2, 0 );
  
  QString s = "trash";
  mFolderTree = new SimpleFolderTree( this, mainFolderTree, s );
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
  QString name = "trash";
  if ( mFolderTree->folder() )
    name = mFolderTree->folder()->idString();
  return name;
}

void ASWizVirusRulesPage::processSelectionChange()
{
  emit selectionChanged();
}

#include "antispamwizard.moc"
