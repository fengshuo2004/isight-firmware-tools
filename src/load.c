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
#define TIMEOUT 300

int
load_firmware(struct usb_device *dev, char *firmware)
{
	int fd, len, req, llen, res, ret = -1;
	unsigned char data[4], rdata[1024], *ptr;
	usb_dev_handle *h;

	/* load firmware */
	if (!(h = usb_open(dev))) {
		ift_warning(_("Failed to open device"));
		return -1;
	}

	if ((fd = g_open(firmware, O_RDONLY)) == -1) {
		ift_warning(_("Failed to open firmware"));
		return -1;
	}

	if ((res = usb_control_msg(h, 0x40, 0xA0, 0xe600, 0,
				   "\1", 1, TIMEOUT)) != 1) {
		ift_warning(_("Failed to init firmware loading"));
		close(fd);
		return -1;
	}
	while (1) {
		/* read length and offset */
		if ((len = read(fd, data, 4)) != 4) {
			if (len == 0) {
				break;
				goto end;
			} else {
				ift_warning(_("Reading firmware header chunk "
					      "failed"));
				goto end;
			}
		}

		len =(data[0] << 8) | data[1];
		req = (data[2] << 8) | data[3];

		/* read data */
		if (len < 0 || len >= 1024) {
			ift_warning(_("Invalid firmware data_length %d, "
				      "load aborted\n"),
				    len);
			goto end;
		} else if (read(fd, rdata, len) != len) {
			ift_warning(_("Failed to read firmware data"));
			goto end;
		}

		/* upload to usb bus */
		for (ptr = rdata; len > 0; req += 50, ptr += 50) {
			llen = len > 50 ? 50 : len;
			len -= llen;

			if ((res = usb_control_msg(h, 0x40, 0xA0, req, 0,
						   (char *) ptr, llen,
						   TIMEOUT)) != llen) {
				ift_warning(_("Firmware load req=0x%x failed: %s"),
					    req, strerror(errno));
				goto end;
			}
		}
	}

	ret = 0;

 end:
	if ((res = usb_control_msg(h, 0x40, 0xA0, 0xe600, 0,
				   "\0", 1, TIMEOUT)) != 1) {
		ift_warning(_("Failed to close firmware loading"));
		ret = -1;
	}

	close(fd);
	usb_close(h);

	return ret;
}

struct usb_device*
find_usb_device(const gchar *bus_id, const gchar *dev_id)
{
	struct usb_bus *bi, *bus = NULL;
	struct usb_device *di, *dev = NULL;

	for (bi = usb_busses; bi != NULL; bi = bi->next) {
		if (g_str_equal(bi->dirname, bus_id)) {
			bus = bi;
			break;
		}
	}

	if (!bus) {
		ift_warning(_("USB Bus %s not found"), bus_id);
		return NULL;
	}

	for (di = bus->devices; di != NULL; di = di->next) {
		if (g_str_equal(di->filename, dev_id)) {
			dev = di;
			break;
		}
	}

	if (!dev)
		ift_warning(_("USB device %s not found in bus %s"),
			    dev_id, bus_id);

	return dev;
}

struct usb_device*
find_usb_product(const guint32 vendor_id, const guint32 product_id)
{
	struct usb_bus *bi;
	struct usb_device *di, *dev = NULL;

	for (bi = usb_busses; bi != NULL; bi = bi->next) {
		for (di = bi->devices; di != NULL; di = di->next) {
			if (di->descriptor.idVendor == vendor_id &&
			    di->descriptor.idProduct == product_id) {
				dev = di;
				break;
			}
		}

		if (dev)
			break;
	}

	if (!dev)
		ift_warning(_("USB device 0x%04X:0x%04X not found"),
			    vendor_id, product_id);

	return dev;
}
