#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <string.h>

extern "C" {
#include <mail.h>
}

#include "mutil.h"

void *readFile(const char *file, ULONG *len)
{
    struct stat fs;
    int fdes = open(file, O_RDONLY);
    char *src;

    if (fdes < 0 || stat(file, &fs) < 0)
        return NIL;
	*len = (ULONG)fs.st_size;
    src = (char *)malloc(fs.st_size);
    if (read(fdes, src, fs.st_size) < 0) {
        close(fdes);
        return NIL;
    }
    close(fdes);
    return src;
} 

long writeFile(const char *file, void *src, ULONG len)
{
    int fdes = open(file, O_WRONLY | O_CREAT, S_IRWXU | S_IROTH | S_IRGRP);

    if (fdes < 0)
        return NIL;

    if (write(fdes, src, len) < 0) {
        close(fdes);
        return NIL;
    }
    close(fdes);
    return T;
}

const char *getName()
{
	const char *r = 0;
	struct passwd *pwd = getpwuid(getuid());

	if (pwd) 
		r = (const char *)strtok(pwd->pw_gecos, ",");
	return r;
}

char *unCRLF(char *s)
{
	ULONG j = 0;
	ULONG i = 0;
	char c;
	char *t = (char *)fs_get(sizeof(s));

	while (i < strlen(s)) {
		if ((c = s[i]) == '\015') {
			if ((c = s[i+1]) != '\0' && c == '\012') {
				i++;
				t[j] = '\012';
			} else {
				t[j] = '\015';
				i++;
				j++;
				t[j] = c;
			}
		} else 
			t[j] = c;
		j++;
		i++;
	}
	//fs_give((void **)&s);
	return s = t;
}
	
const char *basename(const char  *path)
{
    const char *r = (const char *)strrchr(path, '/');

    if (!r)
        return path;
    r++;
    if (*r == '\0')
        return 0;
    return r;
}

int fileType(const char *file)
{
    struct stat buf;
    int fdes = open(file, O_RDONLY);
    int size, i;
    unsigned char a[CHKBYTES];

    if (fdes == -1) 
		return BADFILE;
    if (fstat(fdes, &buf) == -1) 
		return BADFILE;
    size = buf.st_size > 512? 512 : buf.st_size;
    read(fdes, a, size);
	close(fdes);
    for (i = 0; i < size; i++)
        if (a[i] > 128)
            return BINARY;
    return TEXT;
}
