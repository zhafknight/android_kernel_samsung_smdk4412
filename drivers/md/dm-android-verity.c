/*
 * Copyright (C) 2015 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/buffer_head.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/device-mapper.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/key.h>
#include <linux/module.h>
#include <linux/mount.h>
#include <linux/namei.h>
#include <linux/of.h>
#include <linux/reboot.h>
#include <linux/string.h>
#include <linux/vmalloc.h>

#include <asm/setup.h>
#include <crypto/hash.h>
#include <crypto/public_key.h>
#include <crypto/sha.h>
#include <keys/asymmetric-type.h>
#include <keys/system_keyring.h>

#include "dm-verity.h"

/*
 * Target parameters:
 *	<key id>	Key id of the public key in the system keyring.
 *			Verity metadata's signature would be verified against
 *			this. If the key id contains spaces, replace them
 *			with '#'.
 *	<block device>	The block device for which dm-verity is being setup.
 */
static int android_verity_ctr(struct dm_target *ti, unsigned argc, char **argv)
{

	int err;

	err = verity_ctr(ti, argc, argv);

	return err;
}

static struct target_type android_verity_target = {
	.name			= "android-verity",
	.version		= {1, 0, 0},
	.module			= THIS_MODULE,
	.ctr			= android_verity_ctr,
	.dtr			= verity_dtr,
	.map			= verity_map,
	.status			= verity_status,
	.ioctl			= verity_ioctl,
	.merge			= verity_merge,
	.iterate_devices	= verity_iterate_devices,
	.io_hints		= verity_io_hints,
};

static int __init dm_android_verity_init(void)
{
	int r;

	r = dm_register_target(&android_verity_target);
	if (r < 0)
		printk(KERN_ERR "register failed %d\n", r);

	return r;
}

static void __exit dm_android_verity_exit(void)
{
	dm_unregister_target(&android_verity_target);
}

module_init(dm_android_verity_init);
module_exit(dm_android_verity_exit);
