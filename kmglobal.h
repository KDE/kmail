/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
//  removed almost everything: Sven Radej <radej@kde.org>

//this could all go int kmkernel.h
#ifndef kmglobal_h
#define kmglobal_h

#define HDR_FROM     0x01
#define HDR_REPLY_TO 0x02
#define HDR_SUBJECT  0x20
#define HDR_NEWSGROUPS  0x40
#define HDR_FOLLOWUP_TO 0x80
#define HDR_IDENTITY 0x100
#define HDR_TRANSPORT 0x200
#define HDR_FCC       0x400
#define HDR_DICTIONARY 0x800
#define HDR_ALL      0xfff

#define HDR_STANDARD (HDR_SUBJECT)

#endif
