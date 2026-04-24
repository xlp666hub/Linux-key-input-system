KERN_DIR := /home/xlp/workspace/kernel

obj-m := input_key.o

all:
	make -C $(KERN_DIR) M=$(PWD) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules

clean:
	make -C $(KERN_DIR) M=$(PWD) clean
