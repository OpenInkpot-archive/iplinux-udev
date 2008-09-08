/*
 * libudev - interface to udev device information
 *
 * Copyright (C) 2008 Kay Sievers <kay.sievers@vrfy.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "libudev.h"
#include "libudev-private.h"
#include "../udev.h"

struct udev {
	int refcount;
	void (*log_fn)(struct udev *udev,
		       int priority, const char *file, int line, const char *fn,
		       const char *format, va_list args);
	char *sys_path;
	char *dev_path;
	char *rules_path;
	int log_priority;
	int run:1;
};

void udev_log(struct udev *udev,
	      int priority, const char *file, int line, const char *fn,
	      const char *format, ...)
{
	va_list args;

	if (priority > udev->log_priority)
		return;

	va_start(args, format);
	udev->log_fn(udev, priority, file, line, fn, format, args);
	va_end(args);
}

static void log_stderr(struct udev *udev,
		       int priority, const char *file, int line, const char *fn,
		       const char *format, va_list args)
{
	fprintf(stderr, "libudev: %s: ", fn);
	vfprintf(stderr, format, args);
}

/**
 * udev_new:
 *
 * Create udev library context.
 *
 * The initial refcount is 1, and needs to be decremented to
 * release the ressources of the udev library context.
 *
 * Returns: a new udev library context
 **/
struct udev *udev_new(void)
{
	struct udev *udev;
	const char *env;
	char *config_file;
	FILE *f;

	udev = malloc(sizeof(struct udev));
	if (udev == NULL)
		return NULL;
	memset(udev, 0x00, (sizeof(struct udev)));

	sysfs_init();

	/* defaults */
	config_file = NULL;
	udev->refcount = 1;
	udev->log_fn = log_stderr;
	udev->log_priority = LOG_ERR;
	udev->run = 1;
	udev->dev_path = strdup(UDEV_PREFIX "/dev");
	udev->sys_path = strdup("/sys");
	config_file = strdup(SYSCONFDIR "/udev/udev.conf");

	if (udev->dev_path == NULL || udev->sys_path == NULL)
		goto err;

	/* settings by environment and config file */
	env = getenv("SYSFS_PATH");
	if (env != NULL) {
		free(udev->sys_path);
		udev->sys_path = strdup(env);
		remove_trailing_chars(udev->sys_path, '/');
	}

	env = getenv("UDEV_RUN");
	if (env != NULL && !string_is_true(env))
		udev->run = 0;

	env = getenv("UDEV_CONFIG_FILE");
	if (env != NULL) {
		free(config_file);
		config_file = strdup(env);
		remove_trailing_chars(config_file, '/');
	}
	if (config_file == NULL)
		goto err;
	f = fopen(config_file, "r");
	if (f != NULL) {
		char line[LINE_SIZE];
		int line_nr = 0;

		while (fgets(line, sizeof(line), f)) {
			size_t len;
			char *key;
			char *val;

			line_nr++;

			/* find key */
			key = line;
			while (isspace(key[0]))
				key++;

			/* comment or empty line */
			if (key[0] == '#' || key[0] == '\0')
				continue;

			/* split key/value */
			val = strchr(key, '=');
			if (val == NULL) {
				err(udev, "missing <key>=<value> in '%s'[%i], skip line\n", config_file, line_nr);
				continue;
			}
			val[0] = '\0';
			val++;

			/* find value */
			while (isspace(val[0]))
				val++;

			/* terminate key */
			len = strlen(key);
			if (len == 0)
				continue;
			while (isspace(key[len-1]))
				len--;
			key[len] = '\0';

			/* terminate value */
			len = strlen(val);
			if (len == 0)
				continue;
			while (isspace(val[len-1]))
				len--;
			val[len] = '\0';

			if (len == 0)
				continue;

			/* unquote */
			if (val[0] == '"' || val[0] == '\'') {
				if (val[len-1] != val[0]) {
					err(udev, "inconsistent quoting in '%s'[%i], skip line\n", config_file, line_nr);
					continue;
				}
				val[len-1] = '\0';
				val++;
			}

			if (strcasecmp(key, "udev_log") == 0) {
				udev->log_priority = log_priority(val);
				continue;
			}
			if (strcasecmp(key, "udev_root") == 0) {
				free(udev->dev_path);
				udev->dev_path = strdup(val);
				remove_trailing_chars(udev->dev_path, '/');
				continue;
			}
			if (strcasecmp(key, "udev_rules") == 0) {
				free(udev->rules_path);
				udev->rules_path = strdup(val);
				remove_trailing_chars(udev->rules_path, '/');
				continue;
			}
		}
		fclose(f);
	}
	free(config_file);

	env = getenv("UDEV_ROOT");
	if (env != NULL) {
		free(udev->dev_path);
		udev->dev_path = strdup(env);
		remove_trailing_chars(udev->dev_path, '/');
	}

	env = getenv("UDEV_LOG");
	if (env != NULL)
		udev->log_priority = log_priority(env);

	if (udev->dev_path == NULL || udev->sys_path == NULL)
		goto err;

	info(udev, "context %p created\n", udev);
	info(udev, "log_priority=%d\n", udev->log_priority);
	info(udev, "dev_path='%s'\n", udev->dev_path);
	if (udev->rules_path != NULL)
		info(udev, "rules_path='%s'\n", udev->rules_path);

	return udev;
err:
	err(udev, "context creation failed\n");
	udev_unref(udev);
	return NULL;
}

/**
 * udev_ref:
 * @udev: udev library context
 *
 * Take a reference of the udev library context.
 *
 * Returns: the passed udev library context
 **/
struct udev *udev_ref(struct udev *udev)
{
	if (udev == NULL)
		return NULL;
	udev->refcount++;
	return udev;
}

/**
 * udev_unref:
 * @udev: udev library context
 *
 * Drop a reference of the udev library context. If the refcount
 * reaches zero, the ressources of the context will be released.
 *
 **/
void udev_unref(struct udev *udev)
{
	if (udev == NULL)
		return;
	udev->refcount--;
	if (udev->refcount > 0)
		return;
	sysfs_cleanup();
	free(udev->dev_path);
	free(udev->sys_path);
	info(udev, "context %p released\n", udev);
	free(udev);
}

/**
 * udev_set_log_fn:
 * @udev: udev library context
 * @log_fn: function to be called for logging messages
 *
 * The built-in logging, which writes to stderr if the
 * LIBUDEV_DEBUG environment variable is set, can be
 * overridden by a custom function, to plug log messages
 * into the users logging functionality.
 *
 **/
void udev_set_log_fn(struct udev *udev,
		     void (*log_fn)(struct udev *udev,
				    int priority, const char *file, int line, const char *fn,
				    const char *format, va_list args))
{
	udev->log_fn = log_fn;
	info(udev, "custom logging function %p registered\n", udev);
}

int udev_get_log_priority(struct udev *udev)
{
	return udev->log_priority;
}

void udev_set_log_priority(struct udev *udev, int priority)
{
	udev->log_priority = priority;
}

const char *udev_get_rules_path(struct udev *udev)
{
	return udev->rules_path;
}

int udev_get_run(struct udev *udev)
{
	return udev->run;
}

/**
 * udev_get_sys_path:
 * @udev: udev library context
 *
 * Retrieve the sysfs mount point. The default is "/sys". For
 * testing purposes, it can be overridden with the environment
 * variable SYSFS_PATH.
 *
 * Returns: the sys mount point
 **/
const char *udev_get_sys_path(struct udev *udev)
{
	if (udev == NULL)
		return NULL;
	return udev->sys_path;
}

/**
 * udev_get_dev_path:
 * @udev: udev library context
 *
 * Retrieve the device directory path. The default value is "/dev",
 * the actual value may be overridden in the udev configuration
 * file.
 *
 * Returns: the device directory path
 **/
const char *udev_get_dev_path(struct udev *udev)
{
	if (udev == NULL)
		return NULL;
	return udev->dev_path;
}