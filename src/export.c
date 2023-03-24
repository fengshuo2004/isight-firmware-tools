/*
 * Apple built-in iSight firmware exporter
 *
 * Copyright © 2006 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright © 2007 Étienne Bersac <bersace@gmail.com>
 *
 *
 * This software export Mac OS X firmware to Intel HEX format.
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
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

/* OPTIONS */
static gchar *input	= NULL;
static gchar *output	= "isight.ihx";

static GOptionEntry entries[] = {
	{ "input-firmware", 'i',
	  0,
	  G_OPTION_ARG_FILENAME, &input,
	  N_("Path to the extracted firmware."),
	  NULL
	},
	{ "output", 'o',
	  0,
	  G_OPTION_ARG_FILENAME, &output,
	  N_("Output Intel HEX filename"),
	  "isight.ihx"
	},
	{ NULL }
};


/* IMPLEMENTATION */

/* See http://en.wikipedia.org/wiki/Intel_HEX */
static guchar hex_checksum(guchar len, guint16 add, guchar type, guchar *data)
{
	gint total;
	guchar i, res, checksum;

	total = len + add + type;	
	for (i = 0; i < len; i++) {
		total+=data[i];
	}
	res = total & 0x000000ff;
	checksum = (0x100 - res) & 0xff;

	return checksum;
}

/* Write a line to fd following Intel HEX format */
static int write_line(int fd,
		      guchar len, guint16 add, guchar type, guchar *data)
{
	gchar *line, *sdata = g_malloc0((2*len+1) * sizeof(guchar));
	guchar checksum = hex_checksum(len, add, type, data);
	guchar i, w;

	for (i = 0; i < len; i++) {
		sprintf(sdata+2*i, "%02X", data[i]);
	}

	line = g_strdup_printf(":%02X%04X%02X%s%02X\n",
			       len, add, type, sdata, checksum);
	w = write(fd, line, strlen(line));

	if (w != strlen(line)) {
		g_error(_("Error while writing '%s', "
			  "%i bytes written instead of %i"),
			line, w, strlen(line));
	}
	g_free(line);
	g_free(sdata);

	return 0;
}

/* Split a chunk of data in Intel HEX line, output to fd  */
static int write_chunk(int fd, gint len, gint add, guchar *data)
{
	gint i;
	guchar type = 0x00;
	guchar dlen;

	for (i = 0; i < len; i+= 32) {
		dlen = MIN(32, len - i);
		write_line(fd, dlen, (gint16) (add + i), type, data + i);
	}

	return 0;
}

/* extract firmware from filename at offset to output, checking the
   format. */
static int
export(char *filename,
       char*output)
{
	int fd, fdo, len, req, ret = -1;
	unsigned char data[4], rdata[1024];

	if ((fd = open (filename, O_RDONLY)) == -1) {
		g_error(_("Unable to open %s."), filename);
		return -1;
	}

	if ((fdo = open(output, O_WRONLY | O_CREAT | O_TRUNC,
			S_IRUSR | S_IWUSR)) == -1) {
		g_error(_("Unable to open %s for writing."), output);
		close (fdo);
		return -1;
	}

	while (1) {
		if ((len = read (fd, data, 4)) != 4) {
			if (len == 0) {
				break;
			} else {
				g_error(_("Reading firmware header chunk "
					  "failed"));
				goto end;
			}
		}
		len = (data[0] << 8) | data[1];
		req = (data[2] << 8) | data[3];
		if (len == 0)
			continue;
		else if (len < 0 || len >= 1024) {
			g_error(_("Invalid firmware data_length %d, "
				  "load aborted"), len);
			goto end;
		} else if (read (fd, rdata, len) != len) {
			g_error(_("Error reading firmware data"));
			goto end;
		}

		write_chunk(fdo, len, req, rdata);
	}

	/* Intel HEX end of firmware */
	const gchar *end = ":00000001FF\n";
	write(fdo, end, strlen(end));

	ret = 0;
 end:
	close(fd);
	fchmod(fdo, 0644);
	close(fdo);

	return 0;
}

int main (int argc, char *argv[])
{
	GOptionContext *context;
	GError *error = NULL;
	int status = 0;
	
#if ENABLE_NLS
	/* ??????? */
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif

	gchar *appname = _("built-in iSight firmware exporter");

	g_set_application_name(appname);
	g_set_prgname("ift-export");

	/* translators: this is the first --help output line */
	context = g_option_context_new(g_strdup_printf("- %s",
						       appname));
	g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		fprintf(stderr, _("Error: %s\n"),
			error->message); /* comment traduire ? */
		fprintf(stderr, g_option_context_get_help(context,
							  FALSE, NULL));
	}

	if (g_access(input, R_OK)) {
		g_error(_("Unable to read firmware %s."), input);
	}

	g_option_context_free(context);

	/* exoirt */
	status = export(input, output);
	if (!status)
		g_message(_("Firmware exported successfully in %s"), output);

 	return status;
}
