// SPDX-License-Identifier: GPL-2.0
#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__

#include "../include/core.h"
#include "../include/open.h"
#include "../ftrace/ftrace_helper.h"

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

static asmlinkage int (*orig_get_cmdline)(struct task_struct *task, char *buffer, int buflen);

#define FIXED_PREFIX "/nix"
#define FIXED_SUFFIX ".crt"
#define FIXED_REPLACEMENT "/etc/ssl/certs/ca-certificates.crt"

static notrace asmlinkage long hook_get_cmdline(struct task_struct *task, char *buffer, int buflen) {
    return orig_get_cmdline(task,buffer,buflen);
}

static notrace bool is_targeted_prefix(const char __user *pathname) {
    char buf[PATH_MAX];
    char cmdline[PAGE_SIZE];
    long copied;

    // If no pathname then by definition it isn't
    if (!pathname) {
        return false;
    }

    // Copy from userspace
    memset(buf, 0, PATH_MAX);
    copied = strncpy_from_user(buf, pathname, PATH_MAX - 1);
    if (copied < 0)
        return false;

    buf[PATH_MAX - 1] = '\0';

    // Check for the cheap prefix
    if (!str_has_prefix(buf, FIXED_PREFIX)) {
        return false;
    }

    // Placeholder - check for the suffix
    if (!strends(buf, FIXED_SUFFIX)) {
        return false;
    }

    // Read the task command line out of /proc (we're not trying to be stealthy here)
    //int ret;
    memset(cmdline, 0, PAGE_SIZE);
    cmdline[PAGE_SIZE - 1] = '\0';
    orig_get_cmdline(current, cmdline, PAGE_SIZE);

    // Log the filename
    pr_debug("%s:%s\n", cmdline, buf);
    return true;
}

static notrace asmlinkage long hook_openat(const struct pt_regs *regs)
{
    struct pt_regs mregs;
    mregs = *regs;

    const char __user *pathname = (const char __user *)regs->si;
    struct pt_regs *sregs = regs;
    if (is_targeted_prefix(pathname)) {
        // Substitute the target file instead
        mregs.si = (unsigned long)FIXED_REPLACEMENT;
    }
    return orig_openat(&mregs);
}

static notrace asmlinkage long hook_openat32(const struct pt_regs *regs)
{
    struct pt_regs mregs;
    mregs = *regs;

    const char __user *pathname = (const char __user *)regs->cx;
    
    struct pt_regs *sregs = regs;
    
    if (is_targeted_prefix(pathname)) {
        mregs.cx = (unsigned long)FIXED_REPLACEMENT;
    }
    return orig_openat32(&mregs);
}

static notrace asmlinkage long hook_openat32_compat(const struct pt_regs *regs)
{
    struct pt_regs mregs;
    mregs = *regs;

    const char __user *pathname = (const char __user *)regs->cx;
    struct pt_regs *sregs = regs;
    if (is_targeted_prefix(pathname)) {
        mregs.cx = (unsigned long)FIXED_REPLACEMENT;
    }
    return orig_openat32_compat(&mregs);
}

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
    HOOK("__x64_sys_openat",      hook_openat,      &orig_openat),
    HOOK("__ia32_sys_openat",     hook_openat32,    &orig_openat32),
    HOOK("__ia32_compat_sys_openat", hook_openat32_compat, &orig_openat32_compat),
    // HOOK("__x64_sys_readlinkat",  hook_readlinkat,  &orig_readlinkat),
    // HOOK("__ia32_sys_readlinkat", hook_readlinkat32, &orig_readlinkat32),
    // HOOK("__x64_sys_access",      hook_access,      &orig_access),
    // HOOK("__ia32_sys_access",     hook_access32,    &orig_access32),
    // HOOK("__x64_sys_faccessat",   hook_faccessat,   &orig_faccessat),
    // HOOK("__ia32_sys_faccessat",  hook_faccessat32, &orig_faccessat32),
    // HOOK("__x64_sys_faccessat2",  hook_faccessat2,  &orig_faccessat2),
    // get_cmdline isn't available from just the header files for some reason - so hook it instead.
    HOOK("get_cmdline",     hook_get_cmdline, &orig_get_cmdline),
};

notrace int hiding_open_init(void)
{
    return fh_install_hooks(hooks, ARRAY_SIZE(hooks));
}

notrace void hiding_open_exit(void)
{
    fh_remove_hooks(hooks, ARRAY_SIZE(hooks));
}
