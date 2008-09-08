/*
 * Copyright (C) 2004-2005 Kay Sievers <kay.sievers@vrfy.org>
 *
 *	This program is free software; you can redistribute it and/or modify it
 *	under the terms of the GNU General Public License as published by the
 *	Free Software Foundation version 2 of the License.
 * 
 *	This program is distributed in the hope that it will be useful, but
 *	WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *	General Public License for more details.
 * 
 *	You should have received a copy of the GNU General Public License along
 *	with this program; if not, write to the Free Software Foundation, Inc.,
 *	51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "udev.h"
#include "udev_selinux.h"

int create_path(struct udev *udev, const char *path)
{
	char p[PATH_SIZE];
	char *pos;
	struct stat stats;
	int ret;

	strlcpy(p, path, sizeof(p));
	pos = strrchr(p, '/');
	if (pos == p || pos == NULL)
		return 0;

	while (pos[-1] == '/')
		pos--;
	pos[0] = '\0';

	dbg(udev, "stat '%s'\n", p);
	if (stat(p, &stats) == 0 && (stats.st_mode & S_IFMT) == S_IFDIR)
		return 0;

	if (create_path(udev, p) != 0)
		return -1;

	dbg(udev, "mkdir '%s'\n", p);
 	selinux_setfscreatecon(udev, p, NULL, S_IFDIR|0755);
	ret = mkdir(p, 0755);
 	selinux_resetfscreatecon(udev);
	if (ret == 0)
		return 0;

	if (errno == EEXIST)
		if (stat(p, &stats) == 0 && (stats.st_mode & S_IFMT) == S_IFDIR)
			return 0;
	return -1;
}

int delete_path(struct udev *udev, const char *path)
{
	char p[PATH_SIZE];
	char *pos;
	int retval;

	strcpy (p, path);
	pos = strrchr(p, '/');
	if (pos == p || pos == NULL)
		return 0;

	while (1) {
		*pos = '\0';
		pos = strrchr(p, '/');

		/* don't remove the last one */
		if ((pos == p) || (pos == NULL))
			break;

		/* remove if empty */
		retval = rmdir(p);
		if (errno == ENOENT)
			retval = 0;
		if (retval) {
			if (errno == ENOTEMPTY)
				return 0;
			err(udev, "rmdir(%s) failed: %s\n", p, strerror(errno));
			break;
		}
		dbg(udev, "removed '%s'\n", p);
	}
	return 0;
}

/* Reset permissions on the device node, before unlinking it to make sure,
 * that permisions of possible hard links will be removed too.
 */
int unlink_secure(struct udev *udev, const char *filename)
{
	int retval;

	retval = chown(filename, 0, 0);
	if (retval)
		err(udev, "chown(%s, 0, 0) failed: %s\n", filename, strerror(errno));

	retval = chmod(filename, 0000);
	if (retval)
		err(udev, "chmod(%s, 0000) failed: %s\n", filename, strerror(errno));

	retval = unlink(filename);
	if (errno == ENOENT)
		retval = 0;

	if (retval)
		err(udev, "unlink(%s) failed: %s\n", filename, strerror(errno));

	return retval;
}

int file_map(const char *filename, char **buf, size_t *bufsize)
{
	struct stat stats;
	int fd;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		return -1;
	}

	if (fstat(fd, &stats) < 0) {
		close(fd);
		return -1;
	}

	*buf = mmap(NULL, stats.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (*buf == MAP_FAILED) {
		close(fd);
		return -1;
	}
	*bufsize = stats.st_size;

	close(fd);

	return 0;
}

void file_unmap(void *buf, size_t bufsize)
{
	munmap(buf, bufsize);
}

/* return number of chars until the next newline, skip escaped newline */
size_t buf_get_line(const char *buf, size_t buflen, size_t cur)
{
	int escape = 0;
	size_t count;

	for (count = cur; count < buflen; count++) {
		if (!escape && buf[count] == '\n')
			break;

		if (buf[count] == '\\')
			escape = 1;
		else
			escape = 0;
	}

	return count - cur;
}