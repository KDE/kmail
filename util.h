#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

extern int windowCount;

char *getGecos();
void removeDirectory(const char *path); // this is *dangerous*

