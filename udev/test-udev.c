/*
 * Copyright (C) 2003-2004 Greg Kroah-Hartman <greg@kroah.com>
 * Copyright (C) 2004-2006 Kay Sievers <kay.sievers@vrfy.org>
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

#include "config.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <syslog.h>
#include <grp.h>

#include "udev.h"
#include "udev_rules.h"
#include "udev_selinux.h"

static void asmlinkage sig_handler(int signum)
{
	switch (signum) {
		case SIGALRM:
			exit(1);
		case SIGINT:
		case SIGTERM:
			exit(20 + signum);
	}
}

int main(int argc, char *argv[])
{
	struct udev *udev;
	struct sysfs_device *dev;
	struct udevice *udevice;
	const char *maj, *min;
	struct udev_rules rules;
	const char *action;
	const char *devpath;
	const char *subsystem;
	struct sigaction act;
	int devnull;
	int retval = -EINVAL;

	udev = udev_new();
	if (udev == NULL)
		exit(1);
	dbg(udev, "version %s\n", VERSION);
	selinux_init(udev);

	/* set std fd's to /dev/null, /sbin/hotplug forks us, we don't have them at all */
	devnull = open("/dev/null", O_RDWR);
	if (devnull >= 0)  {
		if (devnull != STDIN_FILENO)
			dup2(devnull, STDIN_FILENO);
		if (devnull != STDOUT_FILENO)
			dup2(devnull, STDOUT_FILENO);
		if (devnull != STDERR_FILENO)
			dup2(devnull, STDERR_FILENO);
		if (devnull > STDERR_FILENO)
			close(devnull);
	} else {
		err(udev, "open /dev/null failed: %s\n", strerror(errno));
	}

	/* set signal handlers */
	memset(&act, 0x00, sizeof(act));
	act.sa_handler = (void (*)(int)) sig_handler;
	sigemptyset (&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGALRM, &act, NULL);
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);

	/* trigger timeout to prevent hanging processes */
	alarm(UDEV_EVENT_TIMEOUT);

	action = getenv("ACTION");
	devpath = getenv("DEVPATH");
	subsystem = getenv("SUBSYSTEM");
	/* older kernels passed the SUBSYSTEM only as argument */
	if (subsystem == NULL && argc == 2)
		subsystem = argv[1];

	if (action == NULL || subsystem == NULL || devpath == NULL) {
		err(udev, "action, subsystem or devpath missing\n");
		goto exit;
	}

	/* export log_priority , as called programs may want to do the same as udev */
	if (udev_get_log_priority(udev) > 0) {
		char priority[32];

		sprintf(priority, "%i", udev_get_log_priority(udev));
		setenv("UDEV_LOG", priority, 1);
	}

	sysfs_init();
	udev_rules_init(udev, &rules, 0);

	dev = sysfs_device_get(udev, devpath);
	if (dev == NULL) {
		info(udev, "unable to open '%s'\n", devpath);
		goto fail;
	}

	udevice = udev_device_init(udev);
	if (udevice == NULL)
		goto fail;

	/* override built-in sysfs device */
	udevice->dev = dev;
	strlcpy(udevice->action, action, sizeof(udevice->action));

	/* get dev_t from environment, which is needed for "remove" to work, "add" works also from sysfs */
	maj = getenv("MAJOR");
	min = getenv("MINOR");
	if (maj != NULL && min != NULL)
		udevice->devt = makedev(atoi(maj), atoi(min));
	else
		udevice->devt = udev_device_get_devt(udevice);

	retval = udev_device_event(&rules, udevice);

	/* rules may change/disable the timeout */
	if (udevice->event_timeout >= 0)
		alarm(udevice->event_timeout);

	if (retval == 0 && !udevice->ignore_device && udev_get_run(udev))
		udev_rules_run(udevice);

	udev_device_cleanup(udevice);
fail:
	udev_rules_cleanup(&rules);
	sysfs_cleanup();
	selinux_exit(udev);

exit:
	if (retval != 0)
		return 1;
	return 0;
}