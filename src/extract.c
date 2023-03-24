/*
 * Apple built-in iSight firmware extractor
 *
 * Copyright © 2006 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright © 2007 Étienne Bersac <bersace@gmail.com>
 *
 *
 * This software extract firmware from any Mac OS X iSight driver
 * (named AppleUSBVideoSupport).
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
 *
 *
 *
 * 
 * - First version had hardcoded offset, with a SHA1 sum check
 *   (Ronald)
 *   
 * - Second version, multiple sha1sum were handled with their offset
 *   (Étienne)
 *   
 * - Third version, we search for a preambule in driver and extract
 *   the firmware (Étienne, idea from Jeroen van der Vegt)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <unistd.h>
#include <gcrypt.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

/*
 * The vanilla firmware have bugs in USB Interface descriptor
 * values. We override buggy value with correct one. Those value are
 * triplet, we replace 0xFF 0xFF 0xFF by specific value.
 */
struct patch {
	const char* desc;
	long offset;
	unsigned char value[3];
};

/* From linux-2.6:linux/usb/ch9.J */
#define	USB_CLASS_VIDEO			0x0E
#define USB_CLASS_MISC			0xEF
#define USB_PROTOCOL_IFACE_ASSOC	0x01

/* From linux-uvc: uvcvideo.h */
#define SC_VIDEOCONTROL                 0x01
#define SC_VIDEOSTREAMING               0x02
#define SC_VIDEO_INTERFACE_COLLECTION   0x03
#define PC_PROTOCOL_UNDEFINED           0x00

static struct patch patches[] = {
	{ N_("Fix device descriptor"),
	  .offset	= 0x2352,
	  .value	= { USB_CLASS_MISC,
			    0x02,
			    USB_PROTOCOL_IFACE_ASSOC }
	},
	{ N_("Fix interface assocation descriptor"),
	  .offset	= 0x2373,
	  .value	= { 0x08, 0x0b, 0x00 }
	},
	{ N_("Fix video interface collection"),
	  .offset	= 0x2377,
	  .value	= { USB_CLASS_VIDEO,
			    SC_VIDEO_INTERFACE_COLLECTION,
			    PC_PROTOCOL_UNDEFINED }
	},
	{ N_("Fix video streaming device qualifier"),
	  .offset	= 0x2364,
	  .value	= { USB_CLASS_VIDEO,
			    SC_VIDEOSTREAMING,
			    PC_PROTOCOL_UNDEFINED }
	},
	{ N_("Fix video control interface descriptor"),
	  .offset	= 0x2380,
	  .value	= { USB_CLASS_VIDEO,
			    SC_VIDEOCONTROL,
			    PC_PROTOCOL_UNDEFINED }
	},
	{ N_("Fix video streaming interface descriptor"),
	  .offset	= 0x23C6,
	  .value	= { USB_CLASS_VIDEO,
			    SC_VIDEOSTREAMING,
			    PC_PROTOCOL_UNDEFINED }
	},
};

/* OPTIONS */
static gchar *driver_filename	= NULL;
static gchar *firmware_dir	= "/lib/firmware";
static gchar *firmware_filename = "isight.fw";

static GOptionEntry entries[] = {
	{ "apple-driver", 'a',
	  0,
	  G_OPTION_ARG_FILENAME, &driver_filename,
	  N_("Path to the AppleUSBVideoSupport driver file."),
	  NULL
	},
	{ "output-directory", 'd',
	  0,
	  G_OPTION_ARG_FILENAME, &firmware_dir,
	  N_("Output firmware directory"),
	  "/lib/firmware"
	},
	{ "output-filename", 'f',
	  0,
	  G_OPTION_ARG_FILENAME, &firmware_filename,
	  N_("Name of the output firmware file"),
	  "isight.fw"
	},
	{ NULL }
};


/* IMPLEMENTATION */

/*
 * To find the offset in the driver file, use a hexadecimal editor
 * (e.g. ghex) and search for "USBSend". You will find data like
 * this :
 *
 * "start USBSend #4 returned 0x%x.....X.....4.~.........%\"
 *                                                  ^
 *
 * Each period means 0x00. The firmware start at the ^ sign
 * position. Read the offset and store in in this array. The tool must
 * output the sha1sum of the file, use this to fill the .sha1sum
 * field.
 */
static glong
seek_offset(const gchar* filename)
{
	gint fd;
	glong pos;
	/* This is the preambule of the firmware in the driver file */
	const guchar needle[8] = {0xC2, 0x34, 0x06, 0x7E,
				  0x05, 0x00, 0x00, 0x00};
	guint read_len, chunk_len = 8;
	guint offset = 8;
	guchar* chunk = g_new0(guchar, chunk_len);

	if ((fd = g_open (filename, O_RDONLY)) == -1) {
		g_error(_("Unable to open %s."), filename);
		pos = -1;
		goto end;
	}

	for (pos = 0x1000; pos < 0xD000; pos++) {
		if (lseek(fd, pos, SEEK_SET) != pos) {
			g_error(_("Failed to seek %s at position %x."),
				filename, (guint) pos);
			break;
		}

		if ((read_len = read(fd, chunk, chunk_len)) != chunk_len) {
			g_error(_("Failed to read %s at position %x."),
				filename, (guint) pos);
			break;
		}

		if (memcmp(needle, chunk, chunk_len) == 0) {
			pos+= offset;
			g_message(_("Found firmware signature at offset 0x%X."),
				  (guint) pos);
			goto end_fd;
		}
	}
	pos = -1;


 end_fd:
	close(fd);
 end:
	g_free(chunk);
	return pos;
}


/* extract firmware from filename at offset to output, checking the
   format. */
static int
extract(char *filename,
	long offset,
	char*output)
{
	int fd, fdo, len, req, i, ret = -1;
	unsigned char data[4], rdata[1024];

	if ((fd = g_open (filename, O_RDONLY)) == -1) {
		g_error(_("Unable to open %s."), filename);
		return -1;
	} else if (lseek (fd, offset, SEEK_SET) != offset) {
		g_error(_("Failed to seek %s at position %x."),
			filename, (guint) offset);
		close (fd);
		return -1;
	}

	if ((fdo = g_open(output, O_WRONLY | O_CREAT | O_TRUNC,
			  S_IRUSR | S_IWUSR)) == -1) {
		g_error(_("Unable to open %s for writing."), output);
		close (fdo);
		return -1;
	}

	while (1) {
		if ((len = read (fd, data, 4)) != 4) {
			if (len == 0) {
				g_error(_("Unexpected EOS - corrupt driver?"));
				goto end;
			} else {
				g_error(_("Reading firmware header chunk "
					  "failed"));
				goto end;
			}
		}
		len = (data[0] << 8) | data[1];
		req = (data[2] << 8) | data[3];
		if (len == 0x8001) {
			break;	/* success */
		} else if (len == 0)
			continue;
		else if (len < 0 || len >= 1024) {
			g_error(_("Invalid firmware data_length %d, "
				  "load aborted"), len);
			goto end;
		} else if (read (fd, rdata, len) != len) {
			g_error(_("Error reading firmware data"));
			goto end;
		}

		/* write to file */
		i = write(fdo, data, 4);
		i = write(fdo, rdata, len);
	}

	ret = 0;
 end:
	close(fd);
	fchmod(fdo, 0644);
	close(fdo);

	return 0;
}

static gint
firmware_version(int fd, guchar* version)
{
	guint read_len;

	if (lseek(fd, 4, SEEK_SET) == -1)
		g_error(_("Failed to find firmware version, unable to patch it"));
		
	if ((read_len = read(fd, version, 3)) != 3)
		g_error(_("Failed to read firmware version, unable to patch it"));

	g_message(_("Firmware version %d.%d.%d (0x%02X.0x%02X.0x%02X)"),
		  version[0], version[1], version[2],
		  version[0], version[1], version[2]);

	return 0;
}

static int
patch(char *filename)
{
	int fd, i, n, ret = 0;
	guchar version[3];

	if ((fd = g_open(filename, O_RDWR)) == -1) {
		g_error(_("Error while opening firmware file for patching."));
		return -1;
	}

	firmware_version(fd, version);

	n = G_N_ELEMENTS(patches);
	for (i = 0; i < n; i++) {
		g_message(_("Apply patch %i : %s"),
			  i, gettext(patches[i].desc));

		if (lseek(fd, patches[i].offset, SEEK_SET) == -1) {
			g_error(_("Unable to patch the firmware."));
			ret = -1;
			goto end;
		}

		if (write(fd, &patches[i].value, 3) == 0) {
			g_error(_("Failed to write patched value !"));
			ret = -1;
			goto end;
		}
	}

 end:
	close(fd);
	return ret;
}


int main (int argc, char *argv[])
{
	GOptionContext *context;
	GError *error = NULL;
	int status = 0;
	gchar *pathname;
	glong offset;

#if ENABLE_NLS					\
	/* ??????? */
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif

	gchar *appname = _("built-in iSight firmware extractor "
			   "and patcher");

	g_set_application_name(appname);
	g_set_prgname("ift-extract");

	/* translators: this is the first --help output line */
	context = g_option_context_new(g_strdup_printf("- %s",
						       appname));
	/* translators: this is shown at the end of --help output */
	gchar *desc = g_strdup_printf(_("AppleUSBVideoSupport driver is usualy "					"found in /System/Library/Extensions/"
					"IOUSBFamily.kext/Contents/PlugIns/"
					"AppleUSBVideoSupport.kext/Contents/"
					"MacOS/ in your Mac OS X root volume."
					"\n\n"
					"If you have a non-working Apple "
					"driver file, please send it to %s with"
					"machine description and OS X "
					"version."),
				      PACKAGE_BUGREPORT);
	g_option_context_set_description(context, desc);
	g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		fprintf(stderr, _("Error: %s\n"),
			error->message); /* howto translate? */
		fprintf(stderr, g_option_context_get_help(context,
							  FALSE, NULL));
		return 1;
	}

	if (driver_filename == NULL) {
	  fprintf(stderr, _("Option -a is mandatory. See --help.\n"));
	  return 1;
	}

	if (g_access(driver_filename, R_OK | F_OK)) {
		g_error(_("Unable to read driver %s."), driver_filename);
		return 1;
	}

	pathname = g_build_filename(firmware_dir, firmware_filename, NULL);
	g_option_context_free(context);

	/* search */
	offset = seek_offset(driver_filename);

	if (offset <= 0)
		g_error(_("Unable to find firmware in the file."));

	/* extract */
	status = extract(driver_filename, offset, pathname);
	if (!status)
		g_message(_("Firmware extracted successfully in %s"), pathname);

	/* patch */
	status = patch(pathname);
	if (!status)
		g_message(_("Firmware patched successfully"));
	else
		g_error(_("Failed to apply patches to %s"), pathname);

 	return status;
}
