// kfileio.cpp
// Author: Stefan Taferner <taferner@kde.org>

#include <kapp.h>
#include <kmessagebox.h>
#include <qstring.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <qfile.h>
#include <qfileinfo.h>

#include "kfileio.h"
#include <klocale.h>


//-----------------------------------------------------------------------------
static void msgDialog(const QString &msg)
{
  KMessageBox::sorry(0, msg, i18n("File I/O Error"));
}


//-----------------------------------------------------------------------------
QString kFileToString(const QString &aFileName, bool aEnsureNL, bool aVerbose)
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
      msgDialog(i18n("The specified file does not exist:\n%1").arg(aFileName));
    return QString::null;
  }
  if (info.isDir())
  {
    if (aVerbose)
      msgDialog(i18n("This is a directory and not a file:\n%1").arg(aFileName));
    return QString::null;
  }
  if (!info.isReadable())
  {
    if (aVerbose)
      msgDialog(i18n("You do not have read permissions "
				   "to the file:\n%1").arg(aFileName));
    return QString::null;
  }
  if (len <= 0) return QString::null;

  if (!file.open(IO_Raw|IO_ReadOnly))
  {
    if (aVerbose) switch(file.status())
    {
    case IO_ReadError:
      msgDialog(i18n("Could not read file:\n%1").arg(aFileName));
      break;
    case IO_OpenError:
      msgDialog(i18n("Could not open file:\n%1").arg(aFileName));
      break;
    default:
      msgDialog(i18n("Error while reading file:\n%1").arg(aFileName));
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
    QString msg = i18n("Could only read %1 bytes of %2.")
		.arg(readLen).arg(len);
    msgDialog(msg);
    return QString::null;
  }

  return result;
}

//-----------------------------------------------------------------------------
QByteArray kFileToBytes(const QString &aFileName, bool aVerbose)
{
  QByteArray result;
  QFileInfo info(aFileName);
  unsigned int readLen;
  unsigned int len = info.size();
  QFile file(aFileName);

  //assert(aFileName!=NULL);
  if( aFileName == NULL)
    return result;

  if (!info.exists())
  {
    if (aVerbose)
      msgDialog(i18n("The specified file does not exist:\n%1")
		.arg(aFileName));
    return result;
  }
  if (info.isDir())
  {
    if (aVerbose)
      msgDialog(i18n("This is a directory and not a file:\n%1")
		.arg(aFileName));
    return result;
  }
  if (!info.isReadable())
  {
    if (aVerbose)
      msgDialog(i18n("You do not have read permissions "
				   "to the file:\n%1").arg(aFileName));
    return result;
  }
  if (len <= 0) return result;

  if (!file.open(IO_Raw|IO_ReadOnly))
  {
    if (aVerbose) switch(file.status())
    {
    case IO_ReadError:
      msgDialog(i18n("Could not read file:\n%1").arg(aFileName));
      break;
    case IO_OpenError:
      msgDialog(i18n("Could not open file:\n%1").arg(aFileName));
      break;
    default:
      msgDialog(i18n("Error while reading file:\n%1").arg(aFileName));
    }
    return result;
  }

  result.resize(len);
  readLen = file.readBlock(result.data(), len);
  debug( QString( "len %1" ).arg(len));

  if (readLen < len)
  {
    QString msg;
    msg = i18n("Could only read %1 bytes of %2.")
		.arg(readLen).arg(len);
    msgDialog(msg);
    return result;
  }

  return result;
}


//-----------------------------------------------------------------------------
static
bool kBytesToFile(const char* aBuffer, int len,
		   const QString &aFileName, 
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
      str = i18n("File %1 exists.\nDo you want to replace it?")
		  .arg(aFileName);
      rc = KMessageBox::warningContinueCancel(0, 
	   str, i18n("Save to file"), i18n("&Replace"));
      if (rc != KMessageBox::Continue) return FALSE;
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
	rc = KMessageBox::warningContinueCancel(0,
	     i18n("Failed to make a backup copy of %1.\nContinue anyway?")
	     .arg(aFileName),
             i18n("Save to file"), i18n("&Save"));
	if (rc != KMessageBox::Continue) return FALSE;
      }
    }
  }

  if (!file.open(IO_Raw|IO_WriteOnly))
  {
    if (aVerbose) switch(file.status())
    {
    case IO_WriteError:
      msgDialog(i18n("Could not write to file:\n%1").arg(aFileName));
      break;
    case IO_OpenError:
      msgDialog(i18n("Could not open file for writing:\n%1")
		.arg(aFileName));
      break;
    default:
      msgDialog(i18n("Error while writing file:\n%1").arg(aFileName));
    }
    return FALSE;
  }

  writeLen = file.writeBlock(aBuffer, len);

  if (writeLen < 0) 
  {
    msgDialog(i18n("Could not write to file:\n%1").arg(aFileName));
    return FALSE;
  }
  else if (writeLen < len)
  {
    QString msg = i18n("Could only write %1 bytes of %2.")
		.arg(writeLen).arg(len);
    msgDialog(msg);
    return FALSE;
  } 

  return TRUE;
}

bool kCStringToFile(const QCString& aBuffer, const QString &aFileName, 
		   bool aAskIfExists, bool aBackup, bool aVerbose)
{
    return kBytesToFile(aBuffer, aBuffer.length(), aFileName, aAskIfExists,
	aBackup, aVerbose);
}

bool kByteArrayToFile(const QByteArray& aBuffer, const QString &aFileName, 
		   bool aAskIfExists, bool aBackup, bool aVerbose)
{
    return kBytesToFile(aBuffer, aBuffer.size(), aFileName, aAskIfExists,
	aBackup, aVerbose);
}
