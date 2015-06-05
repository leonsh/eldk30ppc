#
# Makefile for the linux MPC5xxx ppc-specific BestComm
# peripheral controller
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definition is now in the main makefile...

EXTRA_CFLAGS = -I$(TOPDIR)/arch/$(ARCH)/5xxx_io/bestcomm/include -I$(TOPDIR)/arch/$(ARCH)/5xxx_io/bestcomm/capi -I$(TOPDIR)/arch/$(ARCH)/5xxx_io/bestcomm/capi/task_api -I./bestcomm/code_dma/image_rtos1

O_TARGET 			:= 5xxx_io.o

subdir-$(CONFIG_MPC5200)	:= bestcomm
obj-$(CONFIG_MPC5200) 		:= bestcomm/bestcomm.o

ifeq ($(CONFIG_MPC5100),y)
obj-$(CONFIG_PPC_5xxx_FEC)	+= fec.o sdma_5100.o sc_task.impl.o
endif

ifeq ($(CONFIG_MPC5200),y)
obj-$(CONFIG_PPC_5xxx_FEC)	+= fec.o sdma.o
endif

# Support for MPC5100 DMA Engine has been removed
obj-$(CONFIG_PPC_5xxx_PSC)	+= psc.o

include $(TOPDIR)/Rules.make
