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
  // read the configuration for the anti spam tools
  ConfigReader reader( toolList );
  reader.readAndMergeConfig();
  toolList = reader.getToolList();

#ifndef NDEBUG
    kdDebug(5006) << endl << "Considered anti spam tools: " << endl;
#endif
  QStringList descriptionList;
  QStringList whatsThisList;
  QValueListIterator<SpamToolConfig> it = toolList.begin();
  while ( it != toolList.end() )
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
    kdDebug(5006) << "Supports Bayes Filter: " << (*it).useBayesFilter() << endl << endl;
#endif
    it++;
  }

  actionCollection = collection;

  setCaption( i18n( "Anti Spam Wizard" ));
  infoPage = new ASWizInfoPage( 0, "" );
  addPage( infoPage, i18n( "Welcome to the KMail Anti Spam Wizard." ));
  programsPage = new ASWizProgramsPage( 0, "", descriptionList, whatsThisList );
  addPage( programsPage, i18n( "Please select the tools to be used by KMail." ));
  rulesPage = new ASWizRulesPage( 0, "", mainFolderTree );
  addPage( rulesPage, i18n( "Please select the filters to be created inside KMail." ));

  connect( programsPage, SIGNAL( selectionChanged( void ) ),
            this, SLOT( checkProgramsSelections( void ) ) );
  connect( rulesPage, SIGNAL( selectionChanged( void) ),
            this, SLOT( checkRulesSelections( void ) ) );

  connect( this, SIGNAL( helpClicked( void) ),
            this, SLOT( slotHelpClicked( void ) ) );

  setNextEnabled( programsPage, false );
  setNextEnabled( rulesPage, false );

  QTimer::singleShot( 1000, this, SLOT( checkToolAvailability( void ) ) );
}


void AntiSpamWizard::accept()
{
  kdDebug( 5006 ) << "Folder name for spam is "
                  << rulesPage->selectedFolderName() << endl;

  KMFilterActionDict dict;

  QValueListIterator<SpamToolConfig> it = toolList.begin();
  while ( it != toolList.end() )
  {
    if ( programsPage->isProgramSelected( (*it).getVisibleName() )
        && rulesPage->pipeRulesSelected() )
    {
      // pipe messages through the anti spam tools,
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

  if ( rulesPage->moveRulesSelected() )
  {
    // Sort out spam depending on header fields set by the tools
    KMFilter* spamFilter = new KMFilter();
    QPtrList<KMFilterAction>* spamFilterActions = spamFilter->actions();
    KMFilterAction* spamFilterAction1 = dict["transfer"]->create();
    spamFilterAction1->argsFromString( rulesPage->selectedFolderName() );
    spamFilterActions->append( spamFilterAction1 );
    KMFilterAction* spamFilterAction2 = dict["set status"]->create();
    spamFilterAction2->argsFromString( "P" );
    spamFilterActions->append( spamFilterAction2 );
    KMSearchPattern* spamFilterPattern = spamFilter->pattern();
    spamFilterPattern->setName( i18n( "Spam handling" ) );
    spamFilterPattern->setOp( KMSearchPattern::OpOr );
    it = toolList.begin();
    while ( it != toolList.end() )
    {
      if ( programsPage->isProgramSelected( (*it).getVisibleName() ) )
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
      it++;
    }
    spamFilter->setApplyOnOutbound( FALSE);
    spamFilter->setApplyOnInbound();
    spamFilter->setApplyOnExplicit();
    spamFilter->setStopProcessingHere( TRUE );
    spamFilter->setConfigureShortcut( FALSE );

    KMKernel::self()->filterMgr()->appendFilter( spamFilter );
  }

  if ( rulesPage->classifyRulesSelected() )
  {
    // Classify messages manually as Spam
    KMFilter* classSpamFilter = new KMFilter();
    classSpamFilter->setIcon( "mark_as_spam" );
    QPtrList<KMFilterAction>* classSpamFilterActions = classSpamFilter->actions();
    KMFilterAction* classSpamFilterActionFirst = dict["set status"]->create();
    classSpamFilterActionFirst->argsFromString( "P" );
    classSpamFilterActions->append( classSpamFilterActionFirst );
    it = toolList.begin();
    while ( it != toolList.end() )
    {
      if ( programsPage->isProgramSelected( (*it).getVisibleName() )
          && (*it).useBayesFilter() )
      {
        KMFilterAction* classSpamFilterAction = dict["execute"]->create();
        classSpamFilterAction->argsFromString( (*it).getSpamCmd() );
        classSpamFilterActions->append( classSpamFilterAction );
      }
      it++;
    }
    KMFilterAction* classSpamFilterActionLast = dict["transfer"]->create();
    classSpamFilterActionLast->argsFromString( rulesPage->selectedFolderName() );
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
    it = toolList.begin();
    while ( it != toolList.end() )
    {
      if ( programsPage->isProgramSelected( (*it).getVisibleName() )
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
        QString( "Filter Action %1" ).arg( classSpamFilterPattern->name() );
    filterNameSpam = filterNameSpam.replace( " ", "_" );
    QString filterNameHam =
        QString( "Filter Action %1" ).arg( classHamFilterPattern->name() );
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
  QValueListIterator<SpamToolConfig> it = toolList.begin();
  while ( it != toolList.end() )
  {
    if ( programsPage->isProgramSelected( (*it).getVisibleName() ) )
      status = true;
    if ( (*it).useBayesFilter() )
      canClassify = true;
    it++;
  }
  rulesPage->allowClassification( canClassify );
  setNextEnabled( programsPage, status );
}


void AntiSpamWizard::checkRulesSelections()
{
  if ( rulesPage->moveRulesSelected() || rulesPage->pipeRulesSelected()
      || rulesPage->classifyRulesSelected() )
  {
    setFinishEnabled( rulesPage, true );
  } else {
    setFinishEnabled( rulesPage, false );
  }
}


void AntiSpamWizard::checkToolAvailability()
{
  KCursorSaver busy(KBusyPtr::busy()); // this can take some time to find the tools

  // checkboxes for the tools
  QValueListIterator<SpamToolConfig> it = toolList.begin();
  while ( it != toolList.end() )
  {
    int rc = checkForProgram( (*it).getExecutable() );
    programsPage->setProgramAsFound( (*it).getVisibleName(), !rc );
    it++;
  }
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
  KApplication::kApplication()->invokeHelp();
// FIXME Here we need to specify the anchor and perhaps explicitly kmail
// as application if it is necessary when running in Kontact
//  KApplication::kApplication()->invokeHelp( "using-kmail", "kmail" );
}


//---------------------------------------------------------------------------
AntiSpamWizard::SpamToolConfig::SpamToolConfig(QString toolId,
      int configVersion,QString name, QString exec,
      QString url, QString filter, QString detection, QString spam, QString ham,
      QString header, QString pattern, bool regExp, bool bayesFilter)
  : id( toolId ), version( configVersion ),
    visibleName( name ), executable( exec ), whatsThisText( url ),
    filterName( filter ), detectCmd( detection ), spamCmd( spam ),
    hamCmd( ham ), detectionHeader( header ), detectionPattern( pattern ),
    useRegExp( regExp ), supportsBayesFilter( bayesFilter )
{
}


//---------------------------------------------------------------------------
AntiSpamWizard::ConfigReader::ConfigReader( QValueList<SpamToolConfig> & configList )
  : toolList( configList ),
    config( "kmail.antispamrc", true )
{}


void AntiSpamWizard::ConfigReader::readAndMergeConfig()
{
  // read the configuration from the global config file
  config.setReadDefaults( true );
  KConfigGroup general( &config, "General" );
  int registeredTools = general.readNumEntry( "tools", 0 );
  for (int i = 1; i <= registeredTools; i++)
  {
    KConfigGroup toolConfig( &config,
      QCString("Spamtool #") + QCString().setNum(i) );
    toolList.append( readToolConfig( toolConfig ) );
  }

  // read the configuration from the user config file
  // and merge newer config data
  config.setReadDefaults( false );
  KConfigGroup user_general( &config, "General" );
  int user_registeredTools = user_general.readNumEntry( "tools", 0 );
  for (int i = 1; i <= user_registeredTools; i++)
  {
    KConfigGroup toolConfig( &config,
      QCString("Spamtool #") + QCString().setNum(i) );
    mergeToolConfig( readToolConfig( toolConfig ) );
  }
  // Make sure to have add least one tool listed even when the
  // config file was not found or whatever went wrong
  if ( registeredTools < 1 && user_registeredTools < 1)
    toolList.append( createDummyConfig() );
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
  return SpamToolConfig( id, version, name, executable, url,
                         filterName, detectCmd, spamCmd, hamCmd,
                         header, pattern, useRegExp, supportsBayes );
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
                        false, true );
}


void AntiSpamWizard::ConfigReader::mergeToolConfig( AntiSpamWizard::SpamToolConfig config )
{
  bool found = false;
  QValueListIterator<SpamToolConfig> it = toolList.begin();
  while ( it != toolList.end() && !found)
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
        toolList.remove( it );
        toolList.append( config );
      }
    }
    else it++;
  }
  if ( !found )
    toolList.append( config );
}


//---------------------------------------------------------------------------
ASWizInfoPage::ASWizInfoPage( QWidget * parent, const char * name )
  : QWidget( parent, name )
{
  QGridLayout *grid = new QGridLayout( this, 1, 1, KDialog::marginHint(),
                                        KDialog::spacingHint() );
  grid->setColStretch( 1, 10 );

  QLabel *introText = new QLabel( this );
  introText->setText( i18n(
    "<p>Here you get some assistance in setting up KMail's filter "
    "rules to use some commonly known anti spam tools.</p>"
    "The wizard can detect the anti spam tools as well as "
    "create filter rules to pipe messages through these tools "
    "and to separate messages classified as spam. "
    "The wizard will not take any existing filter rules into "
    "consideration but will append new rules in any case.</p>"
    ) );
  grid->addWidget( introText, 0, 0 );
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
    programDict.insert( *it1, box );
    ++it1;
  }

  // hint text
  QLabel *introText = new QLabel( this );
  introText->setText( i18n(
    "<p>For these tools it's possible to let the "
    "wizard create filter rules. KMail tried to find them "
    "in the PATH of your system. The result of that search "
    "is shown for each of the tools.</p>"
    ) );
  grid->addWidget( introText, row++, 0 );
}


bool ASWizProgramsPage::isProgramSelected( const QString &visibleName )
{
  if ( programDict[visibleName] )
    return programDict[visibleName]->isChecked();
  else
    return false;
}


void ASWizProgramsPage::setProgramAsFound( const QString &visibleName, bool found )
{
  QCheckBox * box = programDict[visibleName];
  if ( box )
  {
    QString foundText( i18n("(found on this system)") );
    QString notFoundText( i18n("(not found on this system)") );
    QString labelText = visibleName;
    labelText += " ";
    if ( found )
      labelText += foundText;
    else
      labelText += notFoundText;
    box->setText( labelText );
  }
}


void ASWizProgramsPage::processSelectionChange()
{
  emit selectionChanged();
}

//---------------------------------------------------------------------------
ASWizRulesPage::ASWizRulesPage( QWidget * parent, const char * name,
                                  KMFolderTree * mainFolderTree )
  : QWidget( parent, name )
{
  QGridLayout *grid = new QGridLayout( this, 2, 1, KDialog::marginHint(),
                                        KDialog::spacingHint() );

  classifyRules = new QCheckBox( i18n("Classify messages manually as spam / not spam"), this );
  QWhatsThis::add( classifyRules, i18n("Filters to classify messages will be created.") );
  grid->addWidget( classifyRules, 0, 0 );

  pipeRules = new QCheckBox( i18n("Pipe messages through the anti spam tools"), this );
  QWhatsThis::add( pipeRules, i18n("Appropriate filters will be created") );
  grid->addWidget( pipeRules, 1, 0 );

  moveRules = new QCheckBox( i18n("Move detected spam messages to the selected folder"), this );
  QWhatsThis::add( moveRules, i18n("A filter to detect those messages and to move them will be created.") );
  grid->addWidget( moveRules, 2, 0 );

  QString s = "trash";
  folderTree = new SimpleFolderTree( this, mainFolderTree, s );
  grid->addWidget( folderTree, 3, 0 );

  connect( pipeRules, SIGNAL(clicked()),
            this, SLOT(processSelectionChange(void)) );
  connect( classifyRules, SIGNAL(clicked()),
            this, SLOT(processSelectionChange(void)) );
  connect( moveRules, SIGNAL(clicked()),
            this, SLOT(processSelectionChange(void)) );
}


bool ASWizRulesPage::pipeRulesSelected() const
{
  return pipeRules->isChecked();
}


bool ASWizRulesPage::classifyRulesSelected() const
{
  return classifyRules->isChecked();
}


bool ASWizRulesPage::moveRulesSelected() const
{
  return moveRules->isChecked();
}


QString ASWizRulesPage::selectedFolderName() const
{
  QString name = "trash";
  if ( folderTree->folder() )
    name = folderTree->folder()->idString();
  return name;
}


void ASWizRulesPage::processSelectionChange()
{
  emit selectionChanged();
}


void ASWizRulesPage::allowClassification( bool enabled )
{
  if ( enabled )
    classifyRules->setEnabled( true );
  else
  {
    classifyRules->setChecked( false );
    classifyRules->setEnabled( false );
  }
}


#include "antispamwizard.moc"
