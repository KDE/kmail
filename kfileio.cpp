// kfileio.cpp
// Author: Stefan Taferner <taferner@kde.org>

#include <kapp.h>
#include <kapp.h>
#include <kmsgbox.h>
#include <qmessagebox.h>
#include <qstring.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <qfile.h>
#include <qfileinfo.h>

#include "kfileio.h"
#include <klocale.h>


//-----------------------------------------------------------------------------
static void msgDialog(const char* msg, const char* arg=NULL)
{
  QString str;

  if (arg) str.sprintf(msg, arg);
  else str = msg;

  KMsgBox::message(NULL, i18n("File I/O Error"), str,
		   KMsgBox::STOP, i18n("OK"));
}


//-----------------------------------------------------------------------------
QString kFileToString(const char* aFileName, bool aEnsureNL, bool aVerbose)
{
  QCString result;
  QFileInfo info(aFileName);
  unsigned int readLen;
  unsigned int len = info.size();
  QFile file(aFileName);

  //assert(aFileName!=NULL);
  if( aFileName == NULL)
    return "";

  if (!info.exists())
  {
    if (aVerbose)
      msgDialog(i18n("The specified file does not exist:\n%s"),
		aFileName);
    return QString::null;
  }
  if (info.isDir())
  {
    if (aVerbose)
      msgDialog(i18n("This is a directory and not a file:\n%s"),
		aFileName);
    return QString::null;
  }
  if (!info.isReadable())
  {
    if (aVerbose)
      msgDialog(i18n("You do not have read permissions "
				   "to the file:\n%s"), aFileName);
    return QString::null;
  }
  if (len <= 0) return QString::null;

  if (!file.open(IO_Raw|IO_ReadOnly))
  {
    if (aVerbose) switch(file.status())
    {
    case IO_ReadError:
      msgDialog(i18n("Could not read file:\n%s"), aFileName);
      break;
    case IO_OpenError:
      msgDialog(i18n("Could not open file:\n%s"), aFileName);
      break;
    default:
      msgDialog(i18n("Error while reading file:\n%s"),aFileName);
    }
    return QString::null;
  }

  result.resize(len + (int)aEnsureNL + 1);
  readLen = file.readBlock(result.data(), len);
  if (aEnsureNL && result[len-1]!='\n')
  {
    result[len++] = '\n';
    readLen++;
  }
  result[len] = '\0';

  if (readLen < len)
  {
    QString msg;
    msg.sprintf(i18n("Could only read %u bytes of %u."),
		readLen, len);
    msgDialog(msg);
    return QString::null;
  }

  debug("kFileToString: %d bytes read", readLen);
  return result;
}


//-----------------------------------------------------------------------------
static
bool kBytesToFile(const char* aBuffer, int len,
		   const char* aFileName, 
		   bool aAskIfExists, bool aBackup, bool aVerbose)
{
  QFile file(aFileName);
  QFileInfo info(aFileName);
  int writeLen, rc;

  //assert(aFileName!=NULL);
  if(aFileName == NULL)
    return "";

  if (info.exists())
  {
    if (aAskIfExists)
    {
      QString str;
      str.sprintf(i18n(
		  "File %s exists.\nDo you want to replace it ?"),
		  aFileName);
      rc = QMessageBox::information(NULL, i18n("Information"),
	   str, i18n("&OK"), i18n("&Cancel"),
	   QString::null, 1);
      if (rc != 0) return FALSE;
    }
    if (aBackup)
    {
      // make a backup copy
      QString bakName = aFileName;
      bakName += '~';
      unlink(bakName);
      rc = rename(aFileName, bakName);
      if (rc)
      {
	// failed to rename file
	if (!aVerbose) return FALSE;
	rc = QMessageBox::warning(NULL, i18n("Warning"),
	     i18n(
             "Failed to make a backup copy of %s.\nContinue anyway ?"),
	     i18n("&OK"), i18n("&Cancel"), QString::null, 1);
	if (rc != 0) return FALSE;
      }
    }
  }

  if (!file.open(IO_Raw|IO_WriteOnly))
  {
    if (aVerbose) switch(file.status())
    {
    case IO_WriteError:
      msgDialog(i18n("Could not write to file:\n%s"), aFileName);
      break;
    case IO_OpenError:
      msgDialog(i18n("Could not open file for writing:\n%s"),
		aFileName);
      break;
    default:
      msgDialog(i18n("Error while writing file:\n%s"),aFileName);
    }
    return FALSE;
  }

  writeLen = file.writeBlock(aBuffer, len);

  if (writeLen < 0) 
  {
    msgDialog(i18n("Could not write to file:\n%s"), aFileName);
    return FALSE;
  }
  else if (writeLen < len)
  {
    QString msg;
    msg.sprintf(i18n("Could only write %d bytes of %d."),
		writeLen, len);
    msgDialog(msg);
    return FALSE;
  } 

  return TRUE;
}

bool kCStringToFile(const QCString& aBuffer, const char* aFileName, 
		   bool aAskIfExists, bool aBackup, bool aVerbose)
{
    return kBytesToFile(aBuffer, aBuffer.length(), aFileName, aAskIfExists,
	aBackup, aVerbose);
}

bool kByteArrayToFile(const QByteArray& aBuffer, const char* aFileName, 
		   bool aAskIfExists, bool aBackup, bool aVerbose)
{
    return kBytesToFile(aBuffer, aBuffer.size(), aFileName, aAskIfExists,
	aBackup, aVerbose);
}
