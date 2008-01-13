/*
 *  File : snippetsettings.cpp
 *
 *  Author: Robert Gruber <rgruber@users.sourceforge.net>
 *
 *  Copyright: See COPYING file that comes with this distribution
 */

#include <qstring.h>
#include <klineedit.h>
#include <qcheckbox.h>
#include <qbuttongroup.h>

#include "snippetsettings.h"
#include "snippet_widget.h"


SnippetSettings::SnippetSettings( QWidget *parent, QString name )
{
  setObjectName( name );
  setupUi( this );
  _widget = NULL;
}

SnippetSettings::SnippetSettings( SnippetWidget *w, QWidget *parent, QString name )
{
  setObjectName( name );
  setupUi( this );
  _cfg = w->getSnippetConfig();
  _widget = w;
}


SnippetSettings::~SnippetSettings()
{
}


/*!
    \fn SnippetSettings::slotOKClicked()
 */
void SnippetSettings::slotOKClicked()
{
    _cfg->setToolTips(cbToolTip->isChecked());
    _cfg->setDelimiter(leDelimiter->text());
    _cfg->setInputMethod(btnGroup->selectedId());
}


#include "snippetsettings.moc"
