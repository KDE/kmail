/* This file is part of the Calligra project
 * SPDX-FileCopyrightText: 2003 Stefan Taferner <taferner@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-only
 */
#pragma once

using WORD = short;
using DWORD = int;
using LONG = qint32;
using _HANDLE = void *;

using RECT = struct _RECT {
    WORD left;
    WORD top;
    WORD right;
    WORD bottom;
};

using RECTL = struct _RECTL {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
};

using SIZE = struct _SIZE {
    WORD width;
    WORD height;
};

using SIZEL = struct _SIZEL {
    LONG width;
    LONG height;
};

struct WmfEnhMetaHeader {
    DWORD iType; // Record type EMR_HEADER
    DWORD nSize; // Record size in bytes.  This may be greater
    // than the sizeof( ENHMETAHEADER ).
    RECTL rclBounds; // Inclusive-inclusive bounds in device units
    RECTL rclFrame; // Inclusive-inclusive Picture Frame of metafile
    // in .01 mm units
    DWORD dSignature; // Signature.  Must be ENHMETA_SIGNATURE.
    DWORD nVersion; // Version number
    DWORD nBytes; // Size of the metafile in bytes
    DWORD nRecords; // Number of records in the metafile
    WORD nHandles; // Number of handles in the handle table
    // Handle index zero is reserved.
    WORD sReserved; // Reserved.  Must be zero.
    DWORD nDescription; // Number of chars in the unicode description string
    // This is 0 if there is no description string
    DWORD offDescription; // Offset to the metafile description record.
    // This is 0 if there is no description string
    DWORD nPalEntries; // Number of entries in the metafile palette.
    SIZEL szlDevice; // Size of the reference device in pels
    SIZEL szlMillimeters; // Size of the reference device in millimeters
};
#define ENHMETA_SIGNATURE 0x464D4520

struct WmfMetaHeader {
    WORD mtType;
    WORD mtHeaderSize;
    WORD mtVersion;
    DWORD mtSize;
    WORD mtNoObjects;
    DWORD mtMaxRecord;
    WORD mtNoParameters;
};

struct WmfPlaceableHeader {
    DWORD key;
    WORD hmf;
    RECT bbox;
    WORD inch;
    DWORD reserved;
    WORD checksum;
};
#define APMHEADER_KEY 0x9AC6CDD7

struct WmfMetaRecord {
    DWORD rdSize; // Record size ( in words ) of the function
    WORD rdFunction; // Record function number
    WORD rdParm[1]; // WORD array of parameters
};

struct WmfEnhMetaRecord {
    DWORD iType; // Record type EMR_xxx
    DWORD nSize; // Record size in bytes
    DWORD dParm[1]; // DWORD array of parameters
};

