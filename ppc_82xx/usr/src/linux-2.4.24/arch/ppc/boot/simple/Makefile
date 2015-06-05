# This is far from simple, but I couldn't think of a good name.  This is
# for making the 'zImage' or 'zImage.initrd' on a number of targets.
#
# Author: Tom Rini <trini@mvista.com>
#
# 2001-2002 (c) MontaVista, Software, Inc.  This file is licensed under
# the terms of the GNU General Public License version 2.  This program
# is licensed "as is" without any warranty of any kind, whether express
# or implied.
#
# Notes: For machine targets which produce more than one image, define
# ZNETBOOT and ZNETBOOTRD to the image which should be available for
# 'znetboot' and 'znetboot.initrd`

USE_STANDARD_AS_RULE		:= true

# Normally, we use the 'misc-simple.c' file for decompress_kernel and
# whatnot.  Sometimes we need to override this however.
MISC				:= ../common/misc-simple.o ../common/bootinfo.o
# Additionally, we normally don't need to mess with the L2 / L3 caches
# if present on 'classic' PPC.
ifeq ($(CONFIG_6xx),y)
CACHEFLAG			:= -DCLEAR_CACHES=""
endif

ifeq ($(CONFIG_ARCTIC2),y)
ZIMAGE				:= zImage-TREE
ZIMAGEINITRD			:= zImage.initrd-TREE
ZNETBOOT			:= zImage.treeboot
ZNETBOOTRD			:= zImage.initrd.treeboot
TFTPIMAGE			:= /tftpboot/zImage.embedded
MISC				:= misc-embedded.o
endif
ifeq ($(CONFIG_CPCI690),y)
CFLAGS_misc-cpci690.o		+= -I$(TOPDIR)/arch/$(ARCH)/platforms
ZIMAGE				:= zImage-CPCI690
ZIMAGEINITRD			:= zImage.initrd-CPCI690
EXTRA				:= misc-gt64260.o gt64260_stub.o misc-cpci690.o
CACHEFLAG			:= -include clear.S
TFTPIMAGE			:= /tftpboot/zImage.cpci690
endif
ifeq ($(CONFIG_EMBEDDEDBOOT),y)
ZIMAGE				:= zImage-EMBEDDED
ZIMAGEINITRD			:= zImage.initrd-EMBEDDED
TFTPIMAGE			:= /tftpboot/zImage.embedded
MISC				:= misc-embedded.o
endif
ifeq ($(CONFIG_EV64260),y)
ZIMAGE				:= zImage-EV64260
ZIMAGEINITRD			:= zImage.initrd-EV64260
EXTRA				:= misc-gt64260.o misc-ev64260.o
CACHEFLAG			:= -include clear.S
TFTPIMAGE			:= /tftpboot/zImage.ev64260
endif
ifeq ($(CONFIG_PUMA_A),y)
ZIMAGE				:= zImage-PumaA
ZIMAGEINITRD			:= zImage.initrd-PumaA
EXTRA				:= misc-gt64260.o gt64260_stub.o
CACHEFLAG			:= -include clear.S
TFTPIMAGE			:= /tftpboot/zImage.PumaA
endif
ifeq ($(CONFIG_IBM_OPENBIOS),y)
ZIMAGE				:= zImage-TREE
ZIMAGEINITRD			:= zImage.initrd-TREE
END				:= treeboot
ZNETBOOT			:= zImage.treeboot
ZNETBOOTRD			:= zImage.initrd.treeboot
TFTPIMAGE			:= /tftpboot/zImage.embedded
MISC				:= misc-embedded.o
endif
ifeq ($(CONFIG_EBONY),y)
ZIMAGE				:= zImage-TREE
ZIMAGEINITRD			:= zImage.initrd-TREE
END				:= ebony
ENTRYPOINT			:= 0x00800000
ZNETBOOT			:= zImage.treeboot
ZNETBOOTRD			:= zImage.initrd.treeboot
TFTPIMAGE			:= /tftpboot/zImage.$(END)
EXTRA				:= misc-44x.o
endif
ifeq ($(CONFIG_OCOTEA),y)
ZIMAGE				:= zImage-TREE
ZIMAGEINITRD			:= zImage.initrd-TREE
END				:= ocotea
ENTRYPOINT			:= 0x00800000
ZNETBOOT			:= zImage.treeboot
ZNETBOOTRD			:= zImage.initrd.treeboot
TFTPIMAGE			:= /tftpboot/zImage.$(END)
EXTRA				:= misc-44x.o
endif
ifeq ($(CONFIG_GEMINI),y)
ZIMAGE				:= zImage-SMON
ZIMAGEINITRD			:= zImage.initrd-SMON
TFTPIMAGE			:= /tftpboot/zImage.gemini
endif
ifeq ($(CONFIG_K2),y)
ZIMAGE				:= zImage-K2
ZIMAGEINITRD			:= zImage.initrd-K2
EXTRA				:= legacy.o
CACHEFLAG			:= -include clear.S
TFTPIMAGE			:= /tftpboot/zImage.k2
endif
# kbuild-2.4 'feature', only one of these will ever by 'y' at a time.
# The rest will be unset.
ifeq ($(CONFIG_MCPN765)$(CONFIG_MVME5100)$(CONFIG_PRPMC750)$(CONFIG_PRPMC800)$(CONFIG_LOPEC)$(CONFIG_PPLUS),y)
ZIMAGE				:= zImage-PPLUS
ZIMAGEINITRD			:= zImage.initrd-PPLUS
EXTRA				:= ../common/mpc10x_memory.o
TFTPIMAGE			:= /tftpboot/zImage.pplus
ZNETBOOT			:= zImage.pplus
ZNETBOOTRD			:= zImage.initrd.pplus
endif
ifeq ($(CONFIG_PPLUS),y)
EXTRA				:= legacy.o
endif
ifeq ($(CONFIG_PAL4),y)
ZIMAGE				:= zImage-PAL4
ZIMAGEINITRD			:= zImage.initrd-PAL4
CACHEFLAG			:= -include clear.S
TFTPIMAGE			:= /tftpboot/zImage.pal4
endif
ifeq ($(CONFIG_PCORE)$(CONFIG_POWERPMC250),y)
ZIMAGE				:= zImage-PCORE
ZIMAGEINITRD			:= zImage.initrd-PCORE
CACHEFLAG			:= -include clear.S
TFTPIMAGE			:= /tftpboot/zImage.pcore
endif
ifeq ($(CONFIG_PCORE),y)
EXTRA				:= chrpmap.o
endif
ifeq ($(CONFIG_PPMC260),y)
ZIMAGE                          := zImage-PCORE
ZIMAGEINITRD                    := zImage.initrd-PCORE
EXTRA                           := misc-gt64260.o gt64260_stub.o \
				   misc-ppmc260.o gt64260_iic.o
TFTPIMAGE                       := /tftpboot/zImage.pcore
endif
ifeq ($(CONFIG_SANDPOINT),y)
ZIMAGE				:= zImage-SP
ZIMAGEINITRD			:= zImage.initrd-SP
CACHEFLAG			:= -include clear.S
TFTPIMAGE			:= /tftpboot/zImage.sandpoint
endif
ifeq ($(CONFIG_SPRUCE),y)
ZIMAGE				:= zImage-SPRUCE
ZIMAGEINITRD			:= zImage.initrd-SPRUCE
MISC				:= misc-spruce.o
TFTPIMAGE			:= /tftpboot/zImage.spruce
endif
ifeq ($(CONFIG_GLACIER),y)
ZIMAGE				:= zImage-GLACIER
ZIMAGEINITRD			:= zImage.initrd-GLACIER
TFTPIMAGE			:= /tftpboot/zImage.glacier
ZNETBOOT			:= zImage.glacier
ZNETBOOTRD			:= zImage.initrd.glacier
CACHEFLAG			:= -include clear.S
endif
ifeq ($(CONFIG_ICECUBE),y)
ZIMAGE				:= zImage-ICECUBE
ZIMAGEINITRD			:= zImage.initrd-ICECUBE
TFTPIMAGE			:= /tftpboot/zImage.icecube
ZNETBOOT			:= zImage.icecube
ZNETBOOTRD			:= zImage.initrd.icecube
CACHEFLAG			:= -include clear.S
endif
ifeq ($(CONFIG_SMP),y)
TFTPIMAGE			+= .smp
endif
ifeq ($(CONFIG_REDWOOD_4),y)
# This is a treeboot that needs init functions until the
# boot rom is sorted out (i.e. this is short lived)
EXTRA_AFLAGS			:= -Wa,-m405
EXTRA			:= rw4/rw4_init.o rw4/rw4_init_brd.o
endif
ifeq ($(CONFIG_HXEB100),y)
ZIMAGE				:= zImage-HXEB100
ZIMAGEINITRD			:= zImage.initrd-HXEB100
EXTRA				:= misc-gt64260.o gt64260_stub.o \
					misc-hxeb100.o hxeb100-cpu1.o \
					gt64260_iic.o
CACHEFLAG			:= -include clear.S
TFTPIMAGE			:= /tftpboot/zImage.hxeb100
ZNETBOOT			:= zImage.hxeb100
endif

# Setup a default address to put ourselves, change it as needed.
LD_ARGS				= -T ../ld.script -Ttext 0x00800000 -Bstatic
ifdef CONFIG_8xx
LD_ARGS				:= -T ../ld.script -Ttext 0x00380000 -Bstatic
endif
ifeq ($(CONFIG_8260)$(CONFIG_40x),y)
LD_ARGS				:= -T ../ld.script -Ttext 0x00400000 -Bstatic
endif
ifdef CONFIG_440
LD_ARGS				:= -T ../ld.script -Ttext 0x01000000 -Bstatic
endif
OBJCOPY_ARGS			:= -O elf32-powerpc

# head.o and ../common/relocate.o must be at the start.
obj-y				:= head.o ../common/relocate.o $(EXTRA) \
				$(MISC) ../common/misc-common.o \
				../common/string.o ../common/util.o \
				../common/serial_stub.o
obj-$(CONFIG_40x)		+= embed_config.o
obj-$(CONFIG_8xx)		+= embed_config.o
obj-$(CONFIG_8260)		+= embed_config.o
obj-$(CONFIG_BSEIP)		+= iic.o
obj-$(CONFIG_MBX)		+= iic.o
obj-$(CONFIG_RMU)		+= iic.o embed_config.o
obj-$(CONFIG_RPXCLASSIC)	+= iic.o
obj-$(CONFIG_RPXLITE)		+= iic.o
obj-$(CONFIG_XILINX_ML300)	+= xiic_l.o
# Different boards need different serial implementations.
ifeq ($(CONFIG_SERIAL_CONSOLE),y)
ser-$(CONFIG_XILINX_UARTLITE_CONSOLE) := xuartlite_tty.o
ser-$(CONFIG_8xx)		:= m8xx_tty.o
ser-$(CONFIG_8260)		:= m8260_tty.o
ser-$(CONFIG_SERIAL)		:= ../common/ns16550.o
ser-$(CONFIG_GT64260_CONSOLE)	:= gt64260_tty.o
ser-$(CONFIG_PUMA_A)		:= pumaA_tty.o
ser-$(CONFIG_GLACIER)		:= m5xxx_tty.o
ser-$(CONFIG_ICECUBE)		:= m5xxx_tty.o 
endif
obj-y				+= $(ser-y)

LIBS				:= ../lib/zlib.a
ifdef CONFIG_XILINX_UARTLITE_CONSOLE
LIBS				+= $(TOPDIR)/drivers/char/xilinx_uartlite/xuartlite_l.o
endif

# Tools
MKBUGBOOT			:= ../utils/mkbugboot
MKPREP				:= ../utils/mkprep
MKTREE				:= ../utils/mktree

# Additional CFLAGS / AFLAGS
AFLAGS_head.o			+= $(CACHEFLAG)
ifeq ($(CONFIG_XILINX_ML300),y)
CFLAGS_xuartlite_tty.o		+= -I$(TOPDIR)/drivers/char/xilinx_uartlite
EXTRA_CFLAGS			+= -I$(TOPDIR)/arch/ppc/platforms/xilinx_ocp \
					-I$(TOPDIR)/drivers/i2c/xilinx_iic
endif

zvmlinux: $(obj-y) $(LIBS) ../ld.script ../images/vmlinux.gz ../common/dummy.o
	$(OBJCOPY) $(OBJCOPY_ARGS) \
		--add-section=.image=../images/vmlinux.gz \
		--set-section-flags=.image=contents,alloc,load,readonly,data \
		../common/dummy.o image.o
	$(LD) $(LD_ARGS) -o $@ $(obj-y) image.o $(LIBS)
	$(OBJCOPY) $(OBJCOPY_ARGS) $@ $@ -R .comment -R .stab -R .stabstr \
		-R .ramdisk -R .sysmap

zvmlinux.initrd: $(obj-y) $(LIBS) ../ld.script ../images/vmlinux.gz \
		../common/dummy.o
	$(OBJCOPY) $(OBJCOPY_ARGS) \
		--add-section=.ramdisk=../images/ramdisk.image.gz \
		--set-section-flags=.ramdisk=contents,alloc,load,readonly,data \
		--add-section=.image=../images/vmlinux.gz \
		--set-section-flags=.image=contents,alloc,load,readonly,data \
		../common/dummy.o image.o
	$(LD) $(LD_ARGS) -o $@ $(obj-y) image.o $(LIBS)
	$(OBJCOPY) $(OBJCOPY_ARGS) $@ $@ -R .comment -R .stab -R .stabstr \
		-R .sysmap

# Sort-of dummy rules, that let us format the image we want.
zImage: $(ZIMAGE)
	mv zvmlinux ../images/zImage.elf

zImage.initrd: $(ZIMAGEINITRD)
	mv zvmlinux.initrd ../images/zImage.initrd.elf

znetboot: zImage
ifneq ($(ZNETBOOT),)
	cp ../images/$(ZNETBOOT) $(TFTPIMAGE)
else
	cp ../images/zImage.* $(TFTPIMAGE)
endif

znetboot.initrd: zImage.initrd
ifneq ($(ZNETBOOTRD),)
	cp ../images/$(ZNETBOOTRD) $(TFTPIMAGE)
else
	cp ../images/zImage.* $(TFTPIMAGE)
endif

zImage-EBONY: zvmlinux
	ln -sf zImage.elf ../images/zvmlinux.ebony
	$(MKTREE) zvmlinux ../images/zImage.ebony 0x1000000

zImage.initrd-EBONY: zvmlinux.initrd
	$(MKTREE) zvmlinux.initrd ../images/zImage.initrd.ebony 0x1000000

zImage-EMBEDDED: zvmlinux
	ln -sf zImage.elf ../images/zImage.embedded

zImage.initrd-EMBEDDED: zvmlinux.initrd
	ln -sf zImage.initrd.elf ../images/zImage.initrd.embedded

zImage-K2: zvmlinux
	ln -sf zImage.elf ../images/zImage.k2

zImage.initrd-K2: zvmlinux.initrd
	ln -sf zImage.initrd.elf ../images/zImage.initrd.k2

zImage-CPCI690: zvmlinux
	dd if=zvmlinux of=../images/zImage.cpci690 skip=64 bs=1k

zImage.initrd-CPCI690: zvmlinux.initrd
	dd if=zvmlinux.initrd of=../images/zImage.initrd.cpci690 skip=64 bs=1k

zImage-EV64260: zvmlinux
	ln -sf zImage.elf ../images/zImage.ev64260

zImage.initrd-EV64260: zvmlinux.initrd
	ln -sf zImage.initrd.elf ../images/zImage.initrd.ev64260

zImage-PumaA: zvmlinux
	ln -sf zImage.elf ../images/zImage.PumaA

zImage.initrd-PumaA: zvmlinux.initrd
	ln -sf zImage.initrd.elf ../images/zImage.initrd.PumaA

zImage-PAL4: zvmlinux
	mv zvmlinux ../images/zImage.pal4

zImage.initrd-PAL4: zvmlinux.initrd
	mv zvmlinux.initrd ../images/zImage.initrd.pal4

zImage-PCORE: zvmlinux
	dd if=zvmlinux of=../images/zImage.pcore skip=64 bs=1k

zImage.initrd-PCORE: zvmlinux.initrd
	dd if=zvmlinux.initrd of=../images/zImage.initrd.pcore skip=64 bs=1k

zImage-PPLUS: zvmlinux $(MKPREP) $(MKBUGBOOT)
	$(MKPREP) -pbp zvmlinux ../images/zImage.pplus
	$(MKBUGBOOT) zvmlinux ../images/zImage.bugboot

zImage.initrd-PPLUS: zvmlinux.initrd $(MKPREP) $(MKBUGBOOT)
	$(MKPREP) -pbp zvmlinux.initrd ../images/zImage.initrd.pplus
	$(MKBUGBOOT) zvmlinux.initrd ../images/zImage.initrd.bugboot

zImage-SMON: zvmlinux
	dd if=zvmlinux of=../images/zImage.gemini skip=64 bs=1k

zImage.initrd-SMON: zvmlinux.initrd
	dd if=zvmlinux.initrd of=../images/zImage.initrd.gemini skip=64 bs=1k

zImage-SP: zvmlinux
	ln -sf zImage.elf ../images/zImage.sandpoint

zImage.initrd-SP: zvmlinux.initrd
	ln -sf zImage.initrd.elf ../images/zImage.initrd.sandpoint

zImage-SPRUCE: zvmlinux
	$(MKTREE) zvmlinux ../images/zImage.spruce 0x800000

zImage.initrd-SPRUCE: zvmlinux.initrd
	$(MKTREE) zvmlinux.initrd ../images/zImage.initrd.spruce 0x800000

zImage-TREE: zvmlinux
	$(MKTREE) zvmlinux ../images/zImage.treeboot

zImage.initrd-TREE: zvmlinux.initrd
	$(MKTREE) zvmlinux.initrd ../images/zImage.initrd.treeboot

zImage-HXEB100: zvmlinux
	$(MKPREP) -pbp zvmlinux ../images/zImage.hxeb100

zImage.initrd-HXEB100: zvmlinux.initrd
	$(MKPREP) -pbp zvmlinux.initrd ../images/zImage.initrd.hxeb100

zImage-GLACIER: zvmlinux
	dd if=zvmlinux of=../images/zImage.glacier skip=64 bs=1k

zImage.initrd-GLACIER: zvmlinux.initrd
	dd if=zvmlinux.initrd of=../images/zImage.initrd.glacier skip=64 bs=1k

zImage-ICECUBE: zvmlinux
	dd if=zvmlinux of=../images/zImage.icecube skip=64 bs=1k

zImage.initrd-ICECUBE: zvmlinux.initrd
	dd if=zvmlinux.initrd of=../images/zImage.initrd.icecube skip=64 bs=1k

include $(TOPDIR)/Rules.make
