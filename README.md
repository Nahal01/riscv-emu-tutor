# RISC-V emulation in linux
This repository aims at exploring RISC-V emulation in linux. Herein, qemu is utilized as the emulator for RISC-V chips.
We need a linux image for a specific (or generic RISC-V) system. Fortunately, mainline linux kernel added support for this architecture.
There are 2 options:
1. download linux kernel and rebuild to for RISC-V architecture (e.g. ricv32 or ricv64)
2. download prebuilt image from major linux distros (e.g. Ubuntu, Debain etc.)

Herein, option 2 was chosen as 1 requires proper toolchain for building linux (it is a TODO item)

# Preparation
## Prerequistites

```sudo apt-get install qemu-system-misc opensbi u-boot-qemu```

The following packages are needed:
1. `qemu-system-misc` for RISC-V system/chip emulation
2. `opensbi` allows RISC-V chip to run in M-mode (privilaged mode)
3. `u-boot` as the bootloader (similar to GRUB for firmware)

## Installation
The image for Ubuntu distro with RISC-V support can be downloaded from:
https://cdimage.ubuntu.com/ubuntu-server/daily-live/current/plucky-live-server-riscv64.iso

We need to create the disk image on which the OS will be installed. Below, a 16GB file is allocated as for the guest OS:

```dd if=/dev/zero bs=1M of=disk count=1 seek=16383```

To Install linux on `disk` file run:

```
/usr/bin/qemu-system-riscv64 -machine virt -m 4G -smp cpus=2 -nographic \
    -bios /usr/lib/riscv64-linux-gnu/opensbi/generic/fw_jump.bin \
    -kernel /usr/lib/u-boot/qemu-riscv64_smode/u-boot.bin \
    -netdev user,id=net0 \
    -device virtio-net-device,netdev=net0 \
    -drive file=jammy-live-server-riscv64.img,format=raw,if=virtio \
    -drive file=disk,format=raw,if=virtio \
    -device virtio-rng-pci```

After installation is completed, we can bring up linux as the guest OS by qemu:

```
/usr/bin/qemu-system-riscv64 -machine virt -m 4G -smp cpus=2 -nographic     -bios /usr/lib/riscv64-linux-gnu/opensbi/generic/fw_jump.bin     -kernel /usr/lib/u-boot/qemu-riscv64_smode/u-boot.bin     -netdev type=user,id=net0     -device virtio-net-device,netdev=net0     -drive file=disk,format=raw,if=virtio     -device virtio-rng-pci```

## Basic sanity checks

```
aaa@aaa:~$ uname -a
Linux aaa 6.14.0-13-generic #13.2-Ubuntu SMP PREEMPT_DYNAMIC Sun Apr  6 05:26:54 UTC 2025 riscv64 riscv64 riscv64 GNU/Linux```

```
aaa@aaa:~$ lscpu
Architecture:             riscv64
  Byte Order:             Little Endian
CPU(s):                   2
  On-line CPU(s) list:    0,1
Vendor ID:                0x0
  Model name:             -
    CPU family:           0x0
    Model:                0x0
    Thread(s) per core:   1
    Core(s) per socket:   2
    Socket(s):            1```

## Writing a simple program
Herein, gcc is used to compile `hello-rv.c` into assembly:

``` gcc hello-rv.c -S -o out.S ```

out.S assembly output:
```
aaa@aaa:~$ cat out.S
	.file	"hello-rv.c"
	.option pic
	.attribute arch, "rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_zicsr2p0_zifencei2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
	.section	.rodata
	.align	3
.LC0:
	.string	"fib(%d)=%d\n"
	.text
	.align	1
	.globl	main
	.type	main, @function
main:
.LFB0:
	.cfi_startproc
	addi	sp,sp,-32
	.cfi_def_cfa_offset 32
	sd	ra,24(sp)
	sd	s0,16(sp)
	.cfi_offset 1, -8
	.cfi_offset 8, -16
	addi	s0,sp,32
	.cfi_def_cfa 8, 0
	sw	zero,-20(s0)
	sw	zero,-24(s0)
	j	.L2
.L3:
	lw	a4,-20(s0)
	lw	a5,-24(s0)
	mv	a2,a4
	mv	a1,a5
	lla	a0,.LC0
	call	printf@plt
	lw	a5,-20(s0)
	mv	a4,a5
	lw	a5,-24(s0)
	addw	a5,a4,a5
	sw	a5,-20(s0)
	lw	a5,-24(s0)
	addiw	a5,a5,1
	sw	a5,-24(s0)
.L2:
	lw	a5,-24(s0)
	sext.w	a4,a5
	li	a5,9
	ble	a4,a5,.L3
	nop
	nop
	ld	ra,24(sp)
	.cfi_restore 1
	ld	s0,16(sp)
	.cfi_restore 8
	.cfi_def_cfa 2, 32
	addi	sp,sp,32
	.cfi_def_cfa_offset 0
	jr	ra
	.cfi_endproc
.LFE0:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 14.2.0-19ubuntu2) 14.2.0"
	.section	.note.GNU-stack,"",@progbits
```

which verifies that the program is compiled for RISC-V target (i.e. a0-a31 registers, .attribute arch field)

## device drivers and tree
Since the qemu is running a virtual system, all the peripherals are virtual (like `virtio`). Nonetheless, we can peek at device tree source by 

``` aaa@aaa:~$ dtc -s -I fs /proc/device-tree -O dts ```

The output of the above command is in `dts.txt`.

Device tree is part of the linux kernel and is compiled and packaged along with linux image. However, we can add a device treee with `-dts` option of qemu
to overlay the existing one in the kernel. Herein, we're trying to add a simple device driver along with its device tree and pass it to qemu. The device tree is supposed to have some configurations for our custom device driver (i.e. register base address and range).
The device driver can be loaded using `modprobe`.
