#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/syscalls.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Giovani Ferreira");
MODULE_DESCRIPTION("A huehue module to fool your friends");

static char *huehue_meme = ""; /* Path for file */

module_param(huehue_meme, charp, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(huehue_meme, "The location of the huehue file");

static unsigned long **find_sys_call_table(void);
asmlinkage long huehue_open(const char __user *filename, int flags, umode_t mode);

asmlinkage long (*original_sys_open)(const char __user *, int, umode_t);
asmlinkage unsigned long **sys_call_table;

static int __init huehue_init(void)
{
    sys_call_table = find_sys_call_table();

    if(!sys_call_table){
        printk(KERN_ERR "Couldn't find sys_call_table.\n");
        return -EPERM;
    }

    /* Disable Write Protection*/
    (write_cr0(read_cr0() & (~ 0x10000)));

    original_sys_open = (void*) sys_call_table[__NR_open];
    sys_call_table[__NR_open] = (unsigned long*) huehue_open;

    /* Enable Write Protection */
    (write_cr0(read_cr0() | 0x10000));
    
    return 0;
}

/* Replacement for open syscall */
asmlinkage long huehue_open(const char __user *filename, int flags, umode_t mode)
{
    int len = strlen(filename);

    if(strcmp(filename + len - 4, ".mp3")) {
	/* Just pass through to the real sys_open if the extension isn't .mp3 */
	return (*original_sys_open)(filename, flags, mode);
    } else {
	mm_segment_t old_fs;
	long fd;

        /* This commentary is very useful, keeping it */
	/*
	 * sys_open checks to see if the filename is a pointer to user space
	 * memory. When we're hijacking, the filename we pass will be in kernel
	 * memory. To get around this, we juggle some segment registers. I
	 * believe fs is the segment used for user space, and we're temporarily
	 * changing it to be the segment the kernel uses.
	 *
	 * An alternative would be to use read_from_user() and copy_to_user()
	 * and place the rickroll filename at the location the user code passed
	 * in, saving and restoring the memory we overwrite.
	 */
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fd = (*original_sys_open)(huehue_meme, flags, mode);

	/* Restore fs to its original value */
	set_fs(old_fs);

	return fd;
    }
}

static void __exit huehue_cleanup(void)
{
    /* Disable Write Protection*/
    (write_cr0(read_cr0() & (~ 0x10000)));

    sys_call_table[__NR_open] = (unsigned long*) original_sys_open;

    /* Enable Write Protection */
    (write_cr0(read_cr0() | 0x10000));
}

/* Find the system calls table in memory */
static unsigned long **find_sys_call_table() {
    unsigned long offset;
    unsigned long **sct;

    for(offset = PAGE_OFFSET; offset < ULLONG_MAX; offset += sizeof(void *)) {
        sct = (unsigned long **) offset;
        
        if(sct[__NR_close] == (unsigned long *) sys_close)
            return sct;
    }
    
    return NULL;
}

module_init(huehue_init);
module_exit(huehue_cleanup);
