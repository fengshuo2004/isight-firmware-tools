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

#include <glib.h>
#include <usb.h>

#ifndef _LOAD_H_
#define	_LOAD_H_

#define APPLE_VENDOR_ID		0x05AC
#define	ISIGHT_PRODUCT_ID	0x8300

#define	ift_debug(args...)	syslog(LOG_DEBUG, args)
#define	ift_message(args...)	syslog(LOG_INFO, args)
#define	ift_warning(args...)	syslog(LOG_WARNING, args)
#define	ift_error(args...)	({syslog(LOG_ERR, args); exit(-1);})

int			load_firmware(struct usb_device *dev, char *firmware);
struct usb_device*	find_usb_device(const gchar *bus_id, const gchar *dev_id);
struct usb_device*	find_usb_product(const guint32 vendor_id, const guint32 product_id);

#endif
