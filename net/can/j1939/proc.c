/*
 * Copyright (c) 2010-2011 EIA Electronics
 *
 * Authors:
 * Kurt Van Dijck <kurt.van.dijck@eia.be>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the version 2 of the GNU General Public License
 * as published by the Free Software Foundation
 */

#include <linux/version.h>
#include <linux/string.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include "j1939-priv.h"

const char j1939_procname[] = "can-j1939";

static struct proc_dir_entry *rootdir;

static int j1939_proc_open(struct inode *inode, struct file *file)
{
	int (*fn)(struct seq_file *sqf, void *v) = PDE_DATA(inode);

	return single_open(file, fn, NULL);
}

/* copied from fs/proc/generic.c */
static const struct file_operations j1939_proc_ops = {
	.owner		= THIS_MODULE,
	.open		= j1939_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

int j1939_proc_add(const char *file,
		int (*seq_show)(struct seq_file *sqf, void *v))
{
	struct proc_dir_entry *pde;
	int mode = 0;

	if (seq_show)
		mode |= 0444;

	if (!rootdir)
		return -ENODEV;
	pde = proc_create_data(file, mode, rootdir, &j1939_proc_ops, seq_show);
	if (!pde)
		goto fail_create;
	return 0;

fail_create:
	return -ENOENT;
}
EXPORT_SYMBOL(j1939_proc_add);

void j1939_proc_remove(const char *file)
{
	remove_proc_entry(file, rootdir);
}
EXPORT_SYMBOL(j1939_proc_remove);

__init int j1939_proc_module_init(void)
{
	/* create /proc/net/can directory */
	rootdir = proc_mkdir(j1939_procname, init_net.proc_net);
	if (!rootdir)
		return -EINVAL;
	return 0;
}

void j1939_proc_module_exit(void)
{
	if (rootdir)
		proc_remove(rootdir);
}

