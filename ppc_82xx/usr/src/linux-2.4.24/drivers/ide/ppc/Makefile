
O_TARGET := idedriver-ppc.o

obj-y		:=
obj-m		:=

obj-$(CONFIG_BLK_DEV_CPCI405_IDE)	+= cpci405ide.o
obj-$(CONFIG_BLK_DEV_MPC8xx_IDE)	+= mpc8xx.o
obj-$(CONFIG_BLK_DEV_IDE_PMAC)		+= pmac.o
obj-$(CONFIG_BLK_DEV_IDE_SWARM)		+= swarm.o
+obj-$(CONFIG_BLK_DEV_IDE_MPC5xxx)	+= mpc5xxx_ide.o mpc5xxx_ide_iops.o
 

EXTRA_CFLAGS	:= -I../

ifeq ($(CONFIG_MPC5200),y)
EXTRA_CFLAGS	+= -I$(TOPDIR)/arch/$(ARCH)/5xxx_io/bestcomm/include	\
			-I$(TOPDIR)/arch/$(ARCH)/5xxx_io/bestcomm/capi	\
			-I$(TOPDIR)/arch/$(ARCH)/5xxx_io/bestcomm/capi/task_api \
			-I$(TOPDIR)/arch/$(ARCH)/5xxx_io/bestcomm/code_dma/image_rtos1
endif

include $(TOPDIR)/Rules.make
