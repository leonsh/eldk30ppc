#
# Makefile for the linux MPC5200 bestcomm microcoded DMA Engine
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definition is now in the main makefile...

EXTRA_CFLAGS = -I$(TOPDIR)/arch/$(ARCH)/5xxx_io/bestcomm/include \
	       -I$(TOPDIR)/arch/$(ARCH)/5xxx_io/bestcomm/capi \
	       -I$(TOPDIR)/arch/$(ARCH)/5xxx_io/bestcomm/capi/task_api \
	       -I$(TOPDIR)/arch/$(ARCH)/5xxx_io/bestcomm/code_dma/image_rtos1

O_TARGET := bestcomm.o

obj-y	:=	capi/bestcomm_api.o capi/load_task.o capi/tasksetup.o \
		capi/task_api/tasksetup_bdtable.o \
		code_dma/image_rtos1/task_capi/tasksetup_ata.o \
		code_dma/image_rtos1/task_capi/tasksetup_crc16_dp_0.o \
		code_dma/image_rtos1/task_capi/tasksetup_crc16_dp_1.o \
		code_dma/image_rtos1/task_capi/tasksetup_fec_rx_bd.o \
		code_dma/image_rtos1/task_capi/tasksetup_fec_tx_bd.o \
		code_dma/image_rtos1/task_capi/tasksetup_gen_dp_0.o \
		code_dma/image_rtos1/task_capi/tasksetup_gen_dp_1.o \
		code_dma/image_rtos1/task_capi/tasksetup_gen_dp_2.o \
		code_dma/image_rtos1/task_capi/tasksetup_gen_dp_3.o \
		code_dma/image_rtos1/task_capi/tasksetup_gen_dp_bd_0.o \
		code_dma/image_rtos1/task_capi/tasksetup_gen_dp_bd_1.o \
		code_dma/image_rtos1/task_capi/tasksetup_gen_rx_bd.o \
		code_dma/image_rtos1/task_capi/tasksetup_gen_tx_bd.o \
		code_dma/image_rtos1/task_capi/tasksetup_lpc.o \
		code_dma/image_rtos1/task_capi/tasksetup_pci_rx.o \
		code_dma/image_rtos1/task_capi/tasksetup_pci_tx.o \
		code_dma/image_rtos1/dma_image.o \
		code_dma/image_rtos1/dma_image.reloc.o

include $(TOPDIR)/Rules.make

