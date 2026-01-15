// SPDX-License-Identifier: GPL-2.0
#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__

// #include "include/reset_tainted.h"
// #include "include/become_root.h"
// #include "include/hiding_directory.h"
// #include "include/hiding_stat.h"
// #include "include/hiding_tcp.h"
// #include "include/hooking_insmod.h"
// #include "include/hooks_write.h"
// #include "include/clear_taint_dmesg.h"
// #include "include/hiding_chdir.h"
// #include "include/hiding_readlink.h"
// #include "include/hide_module.h"
// #include "include/bpf_hook.h"
// #include "include/icmp.h"
// #include "include/trace.h"
// #include "include/audit.h"
// #include "include/task.h"

#include "include/core.h"
#include "include/open.h"
#include "path_remapper.h"

MODULE_AUTHOR("Will Rouesnel");
MODULE_DESCRIPTION("VFS Path-Remapper");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0");

static int majorNumber;
static char *message;
static int open_count = 0;
static struct class *path_remapper_class;
static struct device *path_remapper_device; 

static int devnode_open(struct inode *, struct file *);
static int devnode_release(struct inode *, struct file *);
static ssize_t devnode_read(struct file *, char *, size_t, loff_t *); 
static ssize_t devnode_write(struct file *, const char *, size_t, loff_t *); 

static struct file_operations fops = {
	.open = devnode_open,
	.read = devnode_read,
	.write = devnode_write,
	.release = devnode_release,
};

static int __init path_remapper_init(void) {
    int ret = 0;
    // ret |= reset_tainted_init();
    ret |= hiding_open_init();
    // ret |= become_root_init();
    // ret |= hiding_directory_init();
    // ret |= hiding_stat_init();
    // ret |= hiding_tcp_init();
    // ret |= hooking_insmod_init();
    // ret |= clear_taint_dmesg_init();
    // ret |= hooks_write_init();
    // ret |= hiding_chdir_init();
    // ret |= hiding_readlink_init();
    // ret |= bpf_hook_init();
    // ret |= hiding_icmp_init();
    // ret |= trace_pid_init();
    // ret |= hooking_audit_init();
    // ret |= taskstats_hook_init();
    // module_hide_current();
	if (ret != 0)
		return ret;

	pr_info("Loaded");
	majorNumber = register_chrdev(0,DEVICE_NAME, &fops);
	if (majorNumber < 0){
		pr_alert("Problem registering device...\n");
		return majorNumber;
	}	
	pr_info("Device registered successfully\n");
	path_remapper_class = class_create(CLASS_NAME);

	if (IS_ERR(path_remapper_class)){
		unregister_chrdev(majorNumber,DEVICE_NAME);
		pr_alert("Failed to register device\n");
		return PTR_ERR(path_remapper_class);
	}
	path_remapper_device = device_create(path_remapper_class,NULL,MKDEV(majorNumber,0), NULL, DEVICE_NAME);
	if (IS_ERR(path_remapper_device)){
		class_destroy(path_remapper_class);
		unregister_chrdev(majorNumber,DEVICE_NAME);
		pr_alert("Failed to register device\n");
		return PTR_ERR(path_remapper_class);
	}
	pr_info("Device has been successfully created \n");
	message = (char*) kmalloc(sizeof(char)*MESSAGE_LEN,GFP_KERNEL);
	memset(message,0,sizeof(char)*MESSAGE_LEN);

    return ret;
}

static void __exit path_remapper_exit(void) {
    // clear_taint_dmesg_exit();
    // hooking_insmod_exit();
    // hiding_tcp_exit();
    // hiding_stat_exit();
    // hiding_directory_exit();
    // become_root_exit();
    // reset_tainted_exit();
    // hooks_write_exit();
    // hiding_chdir_exit();
    // hiding_readlink_exit();
    hiding_open_exit();
    // bpf_hook_exit();
    // hiding_icmp_exit();
    // trace_pid_cleanup();
    // hooking_audit_exit();
    // taskstats_hook_exit();
    device_destroy(path_remapper_class,MKDEV(majorNumber,0));
	class_unregister(path_remapper_class);
	class_destroy(path_remapper_class);
	unregister_chrdev(majorNumber,DEVICE_NAME);
	kfree(message);
	pr_info("Unloaded and device destroyed...\n");

}

static int devnode_open(struct inode * inode, struct file * filep){
	open_count++;
	return 0;	
}

static ssize_t devnode_read(struct file * filep, char * buffer, size_t len, loff_t * offset){
	int error_count = 0;
	error_count = copy_to_user(buffer,message,len); //copy out of message into buffer

	if (error_count == 0){
		pr_info("Buffer copied to message holder\n");
		return len==0;
	}
	else{
		pr_info("Buffer could not be copied\n");
		return -EFAULT;
	}
	
}

static ssize_t devnode_write(struct file * filep, const char *buffer, size_t len, loff_t *offset){
	if (copy_from_user(message,buffer,len) == 0){ //no check to see if message is big enough
		pr_info("Message successfully copied message => [%s]", message);
		return strlen(message);	
	}else{
		pr_alert("Problem copying message...\n");
		return -EFAULT;
	}
}

static int devnode_release(struct inode *inodep, struct file *filep){
	pr_info("Device released \n");
	return 0;	
}

module_init(path_remapper_init);
module_exit(path_remapper_exit);
