/*
 * Apple built-in iSight firmware loader
 *
 * Copyright © 2006 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright © 2007 Étienne Bersac <bersace@gmail.com>
 *
 *
 * Special thanks to Johannes Berg <johannes@sipsolutions.net> for helping
 * to find out how to load the firmware; see his website on
 * http://johannes.sipsolutions.net/MacBook/iSight for details.
 * Basic structure of firmware data: <len:2><off:2><data:len>, where
 * the $data of size $len is to be put at position $off in the device.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include "load.h"

/* OPTIONS */
static gchar		*firmware	= "isight.fw";

static GOptionEntry entries[] = {
	{ "firmware", 'f',
	  0,
	  G_OPTION_ARG_FILENAME, &firmware,
	  N_("Path to the firmware."),
	  NULL
	},
	{ NULL }
};

int
main(int argc, char *argv[])
{
	GOptionContext *context;
	GError *error = NULL;
	struct usb_device *dev;

	openlog("ift-load", LOG_PERROR, LOG_USER);

#if ENABLE_NLS
	/* ??????? */
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif

	gchar *appname = _("built-in iSight firmware loader");

	g_set_application_name(appname);
	g_set_prgname("ift-load");

	/* translators: this is the first --help output line */
	context = g_option_context_new(g_strdup_printf(" - %s",
						       appname));

	/* tip at the end of --help output */
	gchar *desc = _("Use ift-extract to extract firmware.");
	g_option_context_set_description(context, desc);
	/* translators: this is shown at the end of --help output */
	g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		fprintf(stderr, _("Error: %s\n"),
			error->message); /* comment traduire ? */
		fprintf(stderr, g_option_context_get_help(context,
							  FALSE, NULL));
	}

	if (g_access(firmware, R_OK)) {
		ift_error(_("Unable to read firmware %s."), firmware);
	}

	/* init usb */
	usb_init();
	if (usb_find_busses() == 0) {
		ift_error(_("No USB busses found"));
	} else if (usb_find_devices() == 0) {
		ift_error(_("No USB devices found"));
	}


	dev = find_usb_product(APPLE_VENDOR_ID,
			       ISIGHT_PRODUCT_ID);

	if (!dev)
		ift_error(_("No iSight found"));

	if (load_firmware(dev, firmware) == -1)
		ift_error(_("Failed to upload firmware to 0x%04X:0x%04X"),
			  dev->descriptor.idVendor,
			  dev->descriptor.idProduct);
	else
		ift_message(_("iSight firmware loaded successfully"));

	closelog();

	return 0;
}
