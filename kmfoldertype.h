#ifndef KMFOLDERTYPE_H
#define KMFOLDERTYPE_H

typedef enum
{
  KMFolderTypeMbox = 0,
  KMFolderTypeMaildir,
  KMFolderTypeCachedImap,
  KMFolderTypeImap,
  KMFolderTypeSearch,
  KMFolderTypeUnknown
} KMFolderType;

typedef enum
{
   KMStandardDir = 0,
   KMImapDir,
   KMDImapDir,
   KMSearchDir
} KMFolderDirType;

#endif // KMFOLDERTYPE_H
