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

#ifndef SNIPPETSETTINGS_H
#define SNIPPETSETTINGS_H

#include "snippetsettingsbase.h"

class SnippetWidget;
class SnippetConfig;

/**
This class is the widget that is showen in the
KDevelop settings dialog. It inherits the
class SnippetSettingsBase which is created by the
same named .ui file
@author Robert Gruber
*/
class SnippetSettings : public SnippetSettingsBase
{
Q_OBJECT
public:
    SnippetSettings(QWidget *parent = 0, const char *name = 0);
    SnippetSettings(SnippetWidget * w, QWidget *parent = 0, const char *name = 0);

    ~SnippetSettings();

public slots:
    void slotOKClicked();

private:
  SnippetConfig * _cfg;
  SnippetWidget * _widget;
};

#endif
