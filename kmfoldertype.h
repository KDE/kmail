#ifndef KMFOLDERTYPE_H
#define KMFOLDERTYPE_H

typedef enum
{
  KMFolderTypeMbox = 0,
  KMFolderTypeMaildir,
  KMFolderTypeCachedImap,
  KMFolderTypeImap
} KMFolderType;

typedef enum
{
   KMStandardDir = 0,
   KMImapDir,
   KMSearchDir
} KMFolderDirType;

#endif // KMFOLDERTYPE_H
