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

#endif // KMFOLDERTYPE_H
