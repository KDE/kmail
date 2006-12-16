/*   -*- mode: C++; c-file-style: "gnu" -*-
 *   kmail: KDE mail client
 *   This file: Copyright (C) 2006 Dmitry Morozhnikov
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <config.h>

#ifndef TEMPLATESCONFIGURATIONIMPL_H
#define TEMPLATESCONFIGURATIONIMPL_H

#include "templatesconfiguration.h"

class TemplatesConfiguration: public TemplatesConfigurationStub {
    Q_OBJECT
    
    public:
    
	TemplatesConfiguration(QWidget *parent = 0, const char *name = 0);
	
	void loadFromGlobal();
	void saveToGlobal();
	void loadFromFolder(QString id);
	void saveToFolder(QString id);
	
	void insertCommand(QString cmd, int adjustCursor = 0);
	
	static QString defaultNewMessage();
	static QString defaultReply();
	static QString defaultReplyAll();
	static QString defaultForward();
	static QString defaultQuoteString();
	
    public slots:
    
	void button_insert_clicked();
	
	void slotTextChanged();

    protected:
    
        enum {CDnl = 1, CRem, CInsert, CSystem, CQuotePipe, CQuote, CQHeaders, CHeaders, 
            CTextPipe, CMsgPipe, CBodyPipe, CClearPipe, CText, 
            CToAddr, CToName, CFromAddr, CFromName, CFullSubject, CMsgId, 
            COHeader, CHeader, COToAddr, COToName, COFromAddr, COFromName, COFullSubject, 
            COMsgId, CDateEn, CDateShort, CDate, CDow, CTimeLongEn, CTimeLong, CTime, 
            CODateEn, CODateShort, CODate, CODow, COTimeLongEn, COTimeLong, COTime, 
            CBlank, CNop, CClear, CDebug, CDebugOff, CToFName, CToLName, CFromFName, CFromLName, 
            COToFName, COToLName, COFromFName, COFromLName, CCursor};
    
    signals:
    
        void changed();
    
};

#endif
