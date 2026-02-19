#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/cred.h>
#include <asm/uaccess.h>

#define BUFFER_SIZE 128
#define PROC_NAME "checkroot"

/* To executes when cat /proc/checkroot */
static ssize_t
proc_read(struct file *file,
	  char __user *usr_buf,
	  size_t count,
	  loff_t *ppos)
{
  int rv = 0;
  char buffer[BUFFER_SIZE];

  /* Get creds of the current proc */
  const struct cred *cred = current_cred();

  /* Verify UID si root (0) */
  if (cred->uid.val == 0)
    {
      rv = sprintf(buffer, "You are root :D\n");
      printk(KERN_INFO "checkroot: ROOT access\n");
    }
  else
    {
      rv = sprintf(buffer, "You are not root :P\n");
      printk(KERN_INFO "checkroot: USER access\n");
    }
  
  return simple_read_from_buffer(usr_buf, count, ppos, buffer, rv);
}

static struct proc_ops
proc_ops =
  {
    .proc_read = proc_read
  };

/* Module initiation */
static int __init
checkroot_mod_init(void)
{
  printk(KERN_INFO "Loading Kernel Module∖n");
  if (!proc_create(PROC_NAME, 0666, NULL, &proc_ops))
    return -ENOMEM;
      
  return 0;
}

/* Module exit */
static void __exit
checkroot_mod_exit(void)
{
  printk(KERN_INFO "Removing Kernel Module∖n");
  remove_proc_entry(PROC_NAME, NULL);
}
1
/* Macros for registering module entry and exit points. */
module_init(checkroot_mod_init);
module_exit(checkroot_mod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MF/MI");
MODULE_DESCRIPTION("Check Root Module");
