#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <string.h>
#include <stdarg.h>

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

char *unCRLF(char *, ulong)
{
/*	ULONG j = 0;
	ULONG i = 0;
	char c;
	char *t = (char *)fs_get(sizeof(s));

	while (i < len) {
		if ((c = *s) == '\015') {
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
	fs_give((void **)&s);
	return s = t;*/
	return 0;
}
	
const char *basename(const char *path)
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

    if (fdes == -1 || fstat(fdes, &buf) == -1 || !S_ISREG(buf.st_mode))
		return BADFILE;
    size = buf.st_size > 512? 512 : buf.st_size;
    read(fdes, a, size);
	close(fdes);
    for (i = 0; i < size; i++)
        if (a[i] > 127)
            return BINARY;
    return TEXT;
}

void mdebug(char *fmt, ...)
{
#ifdef DEBUG
	va_list list;
	va_start(list, fmt);
	printf("MCLASS: ");
	vprintf(fmt, list);
	va_end(list);
#endif
}
