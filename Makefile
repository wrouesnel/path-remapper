obj-m += path_remapper.o 

path_remapper-y := module.o \
	modules/open.o \
	ftrace/ftrace_helper.o

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
