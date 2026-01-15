obj-m += path_remapper.o 

path_remapper-y := main.o \
	modules/open.o \
	ftrace/ftrace_helper.o

# singularity-objs := main.o \
#     modules/reset_tainted.o \
#     modules/become_root.o \
#     modules/hiding_directory.o \
#     modules/hiding_tcp.o \
#     modules/hooking_insmod.o \
#     modules/clear_taint_dmesg.o \
#     modules/hidden_pids.o \
#     modules/hiding_stat.o \
#     modules/hooks_write.o \
#     modules/hiding_chdir.o \
#     modules/hiding_readlink.o \
#     modules/bpf_hook.o \
#     modules/icmp.o \
#     modules/audit.o \
#     modules/task.o \
#     modules/hide_module.o modules/trace.o ftrace/ftrace_helper.o

.PHONY: all kmod install fmt

all: kmod path_remapper_app

# Run clang-format on source code
fmt:
	@echo "Running clang-format"
	@clang-format -i $(FILES)

install: kmod
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules_install

debug: kmod
	sudo rmmod path_remapper || true
	sudo insmod path_remapper.ko

kmod:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

path_remapper_app: path_remapper_app.c
	cc -o path_remapper_app $<

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
