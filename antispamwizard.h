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
#ifndef KMAIL_ANTISPAMWIZARD_H
#define KMAIL_ANTISPAMWIZARD_H

#include "kwizard.h"

#include <qcheckbox.h>
#include <qdict.h>
#include <qlabel.h>

class KActionCollection;
class KMFolderTree;

namespace KMail {

  class SimpleFolderTree;
  class SpamToolConfig;
  typedef QValueList<SpamToolConfig> SpamToolConfigList;
  
  class ASWizInfoPage;
  class ASWizProgramsPage;
  class ASWizRulesPage;

  //---------------------------------------------------------------------------
  /**
    @short KMail anti spam wizard.
    @author Andreas Gungl <a.gungl@gmx.de>
   
    The wizard helps to create filter rules to let KMail operate
    with external anti spam tools. The wizard tries to detect the 
    tools, but the user can overide the preselections.
    Then the user can decide what funtionality shall be supported
    by the created filter rules.
    The wizard will append the created filter rules after the
    last existing rule to keep possible conflicts with existing 
    filter configurations minimal.
    
    The configuration for the tools to get checked and set up
    is read fro a config file. The structure of the file is as 
    following:
    <pre>
    [General]
    tools=1
    
    [Spamtool #1]
    VisibleName=&Spamassassin
    Executable=spamassassin -V
    URL=http://spamassassin.org
    PipeFilterName=SpamAssassin Check
    PipeCmdDetect=spamassassin -L
    PipeCmdSpam=sa-learn --spam --single
    PipeCmdHam=sa-learn --ham --single
    DetectionHeader=X-Spam-Flag
    DetectionPattern=yes
    UseRegExp=0
    SupportsBayes=0
    </pre>
    The name of the config file is kmail.antispamrc
    and it's expected in the config dir of KDE.
  */
  class AntiSpamWizard : public KWizard
  {
    Q_OBJECT
    
    public:
      /** Constructor that needs to initialize from the main folder tree 
        of KMail.
        @param parent The parent widget for the wizard.
        @param mainFolderTree The main folder tree from which the folders
          are copied to allow the selection of a spam folder in a tree 
          within one of the wizard pages.
        @param collection In this collection there the wizard will search 
          for the filter menu actions which get created for classification
          rules (to add them later to the main toolbar).
      */
      AntiSpamWizard( QWidget * parent, KMFolderTree * mainFolderTree,
                      KActionCollection * collection );
    
    protected:
      /** Evaluate the settings made and create the appropriate filter rules. */
      void accept();
      
    protected slots:
      /** Modify the status of the wizard to reflect the selection of spam tools. */
      void checkProgramsSelections();
      /** Modify the status of the wizard to reflect the selected functionality. */
      void checkRulesSelections();
      
    private:
      /* The pages in the wizard */
      ASWizInfoPage * infoPage;
      ASWizProgramsPage * programsPage;
      ASWizRulesPage * rulesPage;
      
      /* The configured tools and it's settings to be used in the wizard. */
      SpamToolConfigList toolList;
      
      /* The action collection where the filter menu action is searched in */
      KActionCollection * actionCollection;
  };
  
  
  //---------------------------------------------------------------------------
  /*
    Instances of this class store the settings for one tool as read from
    the config file. Visible name and What's this text can not get 
    translated!
  */
  class SpamToolConfig
  {
    public:
      SpamToolConfig() {};
      SpamToolConfig(QString name, QString exec, QString url, QString filter,
                     QString detection, QString spam, QString ham,
                     QString header, QString pattern, bool regExp,
                     bool bayesFilter );
      
      QString getVisibleName() { return visibleName; };
      QString getExecutable() { return executable; };
      QString getWhatsThisText() { return whatsThisText; };
      QString getFilterName() { return filterName; };
      QString getDetectCmd() { return detectCmd; };
      QString getSpamCmd() { return spamCmd; };
      QString getHamCmd() { return hamCmd; };
      QString getDetectionHeader() { return detectionHeader; };
      QString getDetectionPattern() { return detectionPattern; };
      bool isUseRegExp() { return useRegExp; };
      bool useBayesFilter() { return supportsBayesFilter; };
      
    private:
      // the name as shown by the checkbox in the dialog page
      QString visibleName;
      // the command to check the existance of the tool
      QString executable;
      // the What's This help text (e.g. url for the tool)
      QString whatsThisText;
      // name for the created filter in the filter list
      QString filterName;
      // pipe through cmd used to detect spam messages
      QString detectCmd;
      // pipe through cmd to let the tool learn a spam message
      QString spamCmd;
      // pipe through cmd to let the tool learn a ham message
      QString hamCmd;
      // by which header are messages marked as spam
      QString detectionHeader;
      // what header pattern is used to mark spam messages
      QString detectionPattern;
      // filter searches for the pattern by regExp or contain rule
      bool useRegExp;
      // can the tool learn spam and ham, has it a bayesian algorithm
      bool supportsBayesFilter;
  };

  //---------------------------------------------------------------------------
  class ASWizInfoPage : public QWidget 
  {
    public:
      ASWizInfoPage( QWidget *parent, const char *name );
    
    private:
      QLabel *introText;
  };
  
  //---------------------------------------------------------------------------
  class ASWizProgramsPage : public QWidget 
  {
    Q_OBJECT
    
    public:
      ASWizProgramsPage( QWidget *parent, const char *name, 
                         SpamToolConfigList &toolList );

      bool isProgramSelected( QString visibleName );
    
    private slots:
      void processSelectionChange();
      
    signals:
      void selectionChanged();
      
    private:
      QDict<QCheckBox> programDict;
  };
  
  //---------------------------------------------------------------------------
  class ASWizRulesPage : public QWidget 
  {
    Q_OBJECT
    
    public:
      ASWizRulesPage( QWidget * parent, const char * name, KMFolderTree * mainFolderTree );
    
      bool pipeRulesSelected();
      bool classifyRulesSelected();
      bool moveRulesSelected();
      
      QString selectedFolderName();
      void allowClassification( bool enabled );
      
    private slots:
      void processSelectionChange();
      
    signals:
      void selectionChanged();
      
    private:
      QCheckBox * pipeRules;
      QCheckBox * classifyRules;
      QCheckBox * moveRules;
      SimpleFolderTree *folderTree;
  };


} // namespace KMail

#endif // KMAIL_ANTISPAMWIZARD_H
