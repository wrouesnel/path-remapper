// SPDX-License-Identifier: GPL-2.0
#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__

#include <linux/file.h>
#include "../include/open.h"
#include "../ftrace/ftrace_helper.h"
#include "../utils.h"

static asmlinkage long (*orig_openat)(const struct pt_regs *);
static asmlinkage long (*orig_openat32)(const struct pt_regs *);
static asmlinkage long (*orig_readlinkat)(const struct pt_regs *);
static asmlinkage long (*orig_readlinkat32)(const struct pt_regs *);
static asmlinkage long (*orig_access)(const struct pt_regs *);
static asmlinkage long (*orig_access32)(const struct pt_regs *);
static asmlinkage long (*orig_faccessat)(const struct pt_regs *);
static asmlinkage long (*orig_faccessat32)(const struct pt_regs *);
static asmlinkage long (*orig_faccessat2)(const struct pt_regs *);
static asmlinkage long (*orig_openat32_compat)(const struct pt_regs *);

// from fs/open.c
struct open_flags {
	int open_flag;
	umode_t mode;
	int acc_mode;
	int intent;
	int lookup_flags;
};

/*
 * Unexported kernel functionality we need access to. 
 * These functions are not hooked, but are resolved out of the running kernel.
*/
// getname is because getname is defined in kernel headers for dealing with a kernel string via getname_kernel
// but is not the funciton we want.
static asmlinkage struct filename *(*getname_dyn)(const char __user *name);
static asmlinkage void (*putname_dyn)(struct filename *name);
// get_cmdline_kernel exposes the kernel resolved get_cmdline which differs from the extern in the headers
static asmlinkage int (*get_cmdline_dyn)(struct task_struct *task, char *buffer, int buflen);
static asmlinkage struct open_how (*build_open_how)(int flags, umode_t mode);
static asmlinkage int (*build_open_flags)(const struct open_how *how,
					  struct open_flags *op);
static asmlinkage struct file *(*do_filp_open)(int dfd,
					       struct filename *pathname,
					       const struct open_flags *op);

#define FIXED_PREFIX "/nix"
#define FIXED_SUFFIX ".crt"
#define FIXED_REPLACEMENT "/etc/ssl/certs/ca-certificates.crt"

// POC defines
#define FIXED_PREFIX "/nix"
#define FIXED_SUFFIX ".crt"
#define FIXED_REPLACEMENT "/etc/ssl/certs/ca-certificates.crt"
#define FIXED_TARGET_PROCESS "cat"

static notrace bool is_targeted_prefix(const struct filename *fname)
{
	// If no pathname then by definition it isn't
	if (!fname) {
		return false;
	}

	// TODO: resolve the full path of the target name from the current working directory
	// so we can check if the lookup will work.

	// Check for the cheap prefix
	if (!str_has_prefix(fname->name, FIXED_PREFIX)) {
		return false;
	}

	// Placeholder - check for the suffix
	if (!strends(fname->name, FIXED_SUFFIX)) {
		return false;
	}

	// Read the task command line
	char *cmdline = kzalloc(PAGE_SIZE, GFP_ATOMIC);
	if (!cmdline) {
		return false;
	}

	int res = get_cmdline_dyn(current, cmdline, PAGE_SIZE);
	if (res == 0) {
		kfree(cmdline);
		return false;
	}

	// Log the filename
	pr_info("remap:%s:%s->%s\n",cmdline, fname->name, FIXED_REPLACEMENT);

	kfree(cmdline);
	return true;
}

static notrace asmlinkage long hook_openat(const struct pt_regs *regs)
{
	const int dfd = (const int)regs->di;
	const char __user *filename = (const char __user *)regs->si;
	const int flags = (const int)regs->dx;
	const umode_t mode = (const umode_t)regs->r10;

    struct filename* tmp = getname_dyn(filename);
    if (IS_ERR(tmp)) {
        return PTR_ERR(tmp);
    }

	if (is_targeted_prefix(tmp)) {
	    // reimplement sys_open (which calls openat)
		struct open_how how = build_open_how(flags, mode);
	    // reimplement do_sys_openat2 which is called by sys_open
	    struct open_flags op;
	    int err, fd;

	    err = build_open_flags(&how, &op);
	    if (unlikely(err))
	        return err;

	    struct filename* remapped_path = getname_kernel(FIXED_REPLACEMENT);
	    if (IS_ERR(remapped_path)) {
			return PTR_ERR(remapped_path);
		}
	        
	    fd = get_unused_fd_flags(how.flags);
	    if (likely(fd >= 0)) {
	        struct file *f = do_filp_open(dfd, remapped_path, &op);
	        if (IS_ERR(f)) {
	            put_unused_fd(fd);
	            fd = PTR_ERR(f);
	        } else {
	            fd_install(fd, f);
	        }
	    }

	    putname_dyn(remapped_path);
		putname_dyn(tmp);
    	return fd;
	}
	
    putname_dyn(tmp);
	return orig_openat(regs);
}

// static notrace asmlinkage long hook_openat32(const struct pt_regs *regs)
// {
//     char buf[PATH_MAX];
//     struct pt_regs mregs;
//     mregs = *regs;

//     const char __user *pathname = (const char __user *)regs->cx;
//     if (is_targeted_prefix(pathname)) {
//         mregs.cx = (unsigned long)FIXED_REPLACEMENT;
//     }
//     return orig_openat32(&mregs);
// }

// static notrace asmlinkage long hook_openat32_compat(const struct pt_regs *regs)
// {
//     char buf[PATH_MAX];
//     struct pt_regs mregs;
//     mregs = *regs;

//     const char __user *pathname = (const char __user *)regs->cx;
//     if (is_targeted_prefix(pathname)) {
//         mregs.cx = (unsigned long)FIXED_REPLACEMENT;
//     }
//     return orig_openat32_compat(&mregs);
// }

// static notrace asmlinkage long hook_readlinkat(const struct pt_regs *regs)
// {
//     const char __user *pathname = (const char __user *)regs->si;
//     if (is_targeted_prefix(pathname))
//         return -ENOENT;
//     return orig_readlinkat(regs);
// }

// static notrace asmlinkage long hook_readlinkat32(const struct pt_regs *regs)
// {
//     const char __user *pathname = (const char __user *)regs->cx;
//     if (is_targeted_prefix(pathname))
//         return -ENOENT;
//     return orig_readlinkat32(regs);
// }

// static notrace asmlinkage long hook_access(const struct pt_regs *regs)
// {
//     const char __user *pathname = (const char __user *)regs->di;
//     // if (is_hidden_proc_path(pathname))
//     //     return -ENOENT;
//     return orig_access(regs);
// }

// static notrace asmlinkage long hook_faccessat(const struct pt_regs *regs)
// {
//     const char __user *pathname = (const char __user *)regs->si;
//     // if (is_hidden_proc_path(pathname))
//     //     return -ENOENT;
//     return orig_faccessat(regs);
// }

// static notrace asmlinkage long hook_faccessat2(const struct pt_regs *regs)
// {
//     const char __user *pathname = (const char __user *)regs->si;
//     // if (is_hidden_proc_path(pathname))
//     //     return -ENOENT;
//     return orig_faccessat2(regs);
// }

// static notrace asmlinkage long hook_access32(const struct pt_regs *regs)
// {
//     const char __user *pathname = (const char __user *)regs->bx;
//     // if (is_hidden_proc_path(pathname))
//     //     return -ENOENT;
//     return orig_access32(regs);
// }

// static notrace asmlinkage long hook_faccessat32(const struct pt_regs *regs)
// {
//     const char __user *pathname = (const char __user *)regs->cx;
//     // if (is_hidden_proc_path(pathname))
//     //     return -ENOENT;
//     return orig_faccessat32(regs);
// }

static struct ftrace_hook hooks[] = {
	HOOK("__x64_sys_openat", hook_openat, &orig_openat),
	// HOOK("__ia32_sys_openat",     hook_openat32,    &orig_openat32),
	// HOOK("__ia32_compat_sys_openat", hook_openat32_compat, &orig_openat32_compat),
	// HOOK("__x64_sys_readlinkat",  hook_readlinkat,  &orig_readlinkat),
	// HOOK("__ia32_sys_readlinkat", hook_readlinkat32, &orig_readlinkat32),
	// HOOK("__x64_sys_access",      hook_access,      &orig_access),
	// HOOK("__ia32_sys_access",     hook_access32,    &orig_access32),
	// HOOK("__x64_sys_faccessat",   hook_faccessat,   &orig_faccessat),
	// HOOK("__ia32_sys_faccessat",  hook_faccessat32, &orig_faccessat32),
	// HOOK("__x64_sys_faccessat2",  hook_faccessat2,  &orig_faccessat2),

};

// Unexported functions we would like to use
static struct ftrace_hook funcs[] = {
	HOOK("getname", NULL, &getname_dyn),
	HOOK("putname",     NULL, &putname_dyn),
	HOOK("get_cmdline", NULL, &get_cmdline_dyn),
	HOOK("build_open_how", NULL, &build_open_how),
	HOOK("build_open_flags", NULL, &build_open_flags),
	HOOK("do_filp_open", NULL, &do_filp_open),
};

notrace int hiding_open_init(void)
{
	int ret = 0;
	ret |= fh_resolve_funcs(funcs, ARRAY_SIZE(funcs));
	ret |= fh_install_hooks(hooks, ARRAY_SIZE(hooks));
	return ret;
}

notrace void hiding_open_exit(void)
{
	fh_remove_hooks(hooks, ARRAY_SIZE(hooks));
}
