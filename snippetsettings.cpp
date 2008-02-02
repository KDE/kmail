/***************************************************************************
 *   snippet feature from kdevelop/plugins/snippet/                        *
 *                                                                         * 
 *   Copyright (C) 2007 by Robert Gruber                                   *
 *   rgruber@users.sourceforge.net                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qstring.h>
#include <klineedit.h>
#include <qcheckbox.h>
#include <qbuttongroup.h>

#include "snippetsettings.h"
#include "snippetwidget.h"


SnippetSettings::SnippetSettings(QWidget *parent, const char *name)
 : SnippetSettingsBase(parent, name)
{
  _widget = NULL;
}

SnippetSettings::SnippetSettings(SnippetWidget * w, QWidget *parent, const char *name)
 : SnippetSettingsBase(parent, name)
{
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
