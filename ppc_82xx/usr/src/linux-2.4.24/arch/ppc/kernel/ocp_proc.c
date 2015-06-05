/*
 * FILE NAME: ocp_proc.c
 *
 * BRIEF MODULE DESCRIPTION: 
 * Based on drivers/pci/proc.c, Copyright (c) 1997--1999 Martin Mares
 *
 * Author: Armin akuster@pacbell.net 
 *
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Version 1.0 02/10/02 -Armin
 *  	initial release
 *
 *  Version 1.1 02/13/02 - Armin
 *   	Added emac support
 *
 *  Version 1.2 05/25/02 -Armin
 *      struct name change from ocp_dev to ocp_dev
 *
 *  Version 1.3 05/30/02 - Armin
 *    added ZMII to show funcs.
 *
 *  Version 1.4 06/14/02 - Armin/David Gidson
 *  using OCP_IRQ_MUL for those devices that have
 *  more than one irq .
 *  cleaned the up display format
 *  Added some cleanups from David Gibson
 *
 *  Version 1.5 06/28/02 - Armin
 *  removed use/need of irq_*
 *  
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/seq_file.h>

#include <asm/uaccess.h>
#include <asm/byteorder.h>
#include <asm/ocp.h>
extern struct type_info ocp_type_info[];

/* iterator */
static void *
ocp_seq_start(struct seq_file *m, loff_t * pos)
{
	struct list_head *p = &ocp_list;
	loff_t n = *pos;

	/* XXX: surely we need some locking for traversing the list? */
	while (n--) {
		p = p->next;
		if (p == &ocp_list)
			return NULL;
	}
	return p;
}
static void *
ocp_seq_next(struct seq_file *m, void *v, loff_t * pos)
{
	struct list_head *p = v;
	(*pos)++;
	return p->next != &ocp_list ? p->next : NULL;
}
static void
ocp_seq_stop(struct seq_file *m, void *v)
{
	/* release whatever locks we need */
}

static int
show_device(struct seq_file *m, void *v)
{
	struct list_head *p = v;
	int i;
	const struct ocp_dev *drv;

	if (p == &ocp_list)
		return 0;

	drv = ocp_dev_g(p);
	seq_printf(m, "%s\t %02d", drv->name, drv->num);
	i = ocp_get_irq(drv->type, drv->num);
	if (i == OCP_IRQ_NA) 
		seq_printf(m, " N/A");
	else 
		seq_printf(m, " %02d", drv->irq);

	seq_printf(m, " %lx", drv->paddr);
	if (drv->vaddr)
		seq_printf(m, " %p", drv->vaddr);
	else
		seq_printf(m, " N/A");
	seq_putc(m, '\n');
	return 0;
}

/*
 * Convert some of the configuration space registers of the device at
 * address (bus,devfn) into a string (possibly several lines each).
 * The configuration string is stored starting at buf[len].  If the
 * string would exceed the size of the buffer (SIZE), 0 is returned.
 */
static int
show_config(struct seq_file *m, void *v)
{
	struct list_head *p = v;
	int i;
	const struct ocp_dev *drv;
	if (p == &ocp_list) {
		seq_puts(m, "OCP devices found:\n");
		return 0;
	}
	drv = ocp_dev_g(p);
	seq_printf(m, " Device: %s%02d\n", drv->name, drv->num);
	seq_printf(m, "  description: %s\n", ocp_type_info[drv->type].desc);
	i = ocp_get_irq(drv->type, drv->num);
	if (i == OCP_IRQ_NA) 
		seq_printf(m, "   irq: N/A\n");
	 else 
		seq_printf(m, "   irq: %02d\n", drv->irq);
	

	seq_printf(m, "   physical: 0x%lx\n", drv->paddr);
	if (drv->vaddr)
		seq_printf(m, "   virtual: 0x%p\n\n", drv->vaddr);
	else
		seq_printf(m, "   virtual: N/A\n\n");
	return 0;
}
static struct seq_operations proc_ocp_op = {
	start:ocp_seq_start,
	next:ocp_seq_next,
	stop:ocp_seq_stop,
	show:show_device
};

static struct seq_operations proc_bus_ocp_op = {
	start:ocp_seq_start,
	next:ocp_seq_next,
	stop:ocp_seq_stop,
	show:show_config
};

static int
proc_ocp_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &proc_ocp_op);
}
static struct file_operations proc_ocp_operations = {
	open:proc_ocp_open,
	read:seq_read,
	llseek:seq_lseek,
	release:seq_release,
};

static struct proc_dir_entry *proc_bus_ocp_dir;

int
ocp_proc_attach_device(struct ocp_dev *dev)
{
	struct proc_dir_entry *e;
	char name[16];

	sprintf(name, "%s%d", dev->name, dev->num);
	e = dev->procent =
	    create_proc_entry(name, S_IFREG | S_IRUGO | S_IWUSR,
			      proc_bus_ocp_dir);
	if (!e)
		return -ENOMEM;
	e->proc_fops = &proc_ocp_operations;
	e->data = dev;
	e->size = 256;
	return 0;
}

int
ocp_proc_detach_device(struct ocp_dev *dev)
{
	struct proc_dir_entry *e;

	if ((e = dev->procent)) {
		if (atomic_read(&e->count))
			return -EBUSY;
		remove_proc_entry(e->name, proc_bus_ocp_dir);
		dev->procent = NULL;
	}
	return 0;
}

static int
proc_bus_ocp_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &proc_bus_ocp_op);
}
static struct file_operations proc_bus_ocp_operations = {
	open:proc_bus_ocp_open,
	read:seq_read,
	llseek:seq_lseek,
	release:seq_release,
};

static int __init
ocp_proc_init(void)
{
	struct proc_dir_entry *entry;
	proc_bus_ocp_dir = proc_mkdir("ocp", proc_bus);
	entry = create_proc_entry("devices", 0, proc_bus_ocp_dir);
	if (entry)
		entry->proc_fops = &proc_bus_ocp_operations;
	return 0;
}

__initcall(ocp_proc_init);
