#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

USE_STANDARD_AS_RULE	:= true

ifdef CONFIG_PPC64BRIDGE
EXTRA_AFLAGS		:= -Wa,-mppc64bridge
endif
ifdef CONFIG_40x
EXTRA_AFLAGS		:= -Wa,-m405
endif

# Extra CFLAGS so we don't have to do relative includes
CFLAGS_pmac_setup.o	+= -I$(TOPDIR)/drivers/scsi -I$(TOPDIR)/fs/partitions \
				-I$(TOPDIR)/arch/$(ARCH)/mm

O_TARGET := platform.o

export-objs			:= prep_setup.o beech.o ibm405lp.o arctic2.o \
				   mpc8260_pci9.o ibm440gp.o ibm440gx.o

obj-$(CONFIG_405GP)		+= ibm405gp.o
obj-$(CONFIG_405LP)		+= ibm405lp.o
obj-$(CONFIG_440GP)		+= ibm440gp.o
obj-$(CONFIG_440GX)		+= ibm440gx.o
obj-$(CONFIG_NP405L)		+= ibmnp405l.o
obj-$(CONFIG_NP405H)		+= ibmnp405h.o
obj-$(CONFIG_405EP)		+= ibm405ep.o
obj-$(CONFIG_NP405GS)		+= ibmnp4gs.o

obj-$(CONFIG_CEDER)		+= ceder.o
obj-$(CONFIG_CPCI405)		+= cpci405.o
obj-$(CONFIG_DMC405)		+= dmc405.o ibm405gp.o
obj-$(CONFIG_EVB405EP)		+= evb405ep.o
obj-$(CONFIG_PPCHAMELEONEVB)	+= PPChameleonEVB.o
obj-$(CONFIG_EP405)		+= ep405.o
obj-$(CONFIG_XILINX_ML300)	+= xilinx_ml300.o
subdir-$(CONFIG_XILINX_OCP)	+= xilinx_ocp
obj-$(CONFIG_XILINX_OCP)	+= xilinx_ocp/xilinx_ocp.o
obj-$(CONFIG_REDWOOD_4)		+= redwood.o ibmstb3.o
obj-$(CONFIG_REDWOOD_5)		+= redwood5.o ibmstb4.o
obj-$(CONFIG_REDWOOD_6)		+= redwood6.o ibmstbx25.o
obj-$(CONFIG_WALNUT)		+= walnut.o
obj-$(CONFIG_ASH)		+= ash.o
obj-$(CONFIG_RAINIER)		+= rainier.o
obj-$(CONFIG_BEECH)		+= beech.o
obj-$(CONFIG_ARCTIC2)		+= arctic2.o subzero.o
ifeq ($(CONFIG_405LP),y)
obj-$(CONFIG_PM)		+= ibm405lp_pm.o ibm405lp_pmasm.o
endif
obj-$(CONFIG_SYCAMORE)		+= sycamore.o ibm405gpr.o
obj-$(CONFIG_EXBITGEN)		+= exbitgen.o ibm405gp.o

obj-$(CONFIG_EBONY)		+= ebony.o
obj-$(CONFIG_OCOTEA)		+= ocotea.o

obj-$(CONFIG_APUS)		+= apus_setup.o
ifeq ($(CONFIG_APUS),y)
obj-$(CONFIG_PCI)		+= apus_pci.o
endif
ifeq ($(CONFIG_8260),y)
ifeq ($(CONFIG_ADS8260),y)
obj-$(CONFIG_PCI)		+= mpc8260_pci9.o
else
obj-$(CONFIG_PCI)		+= m8260_pci.o
endif
endif
ifeq ($(CONFIG_ADS8260),y)
obj-$(CONFIG_PCI)		+= mpc8266ads_pci.o
endif
obj-$(CONFIG_ALL_PPC)		+= pmac_pic.o pmac_setup.o pmac_time.o \
					pmac_feature.o pmac_pci.o chrp_setup.o\
					chrp_time.o chrp_pci.o prep_pci.o \
					prep_time.o prep_setup.o pmac_sleep.o \
					pmac_nvram.o
obj-$(CONFIG_PMAC_BACKLIGHT)	+= pmac_backlight.o
obj-$(CONFIG_PPC_RTAS)		+= error_log.o proc_rtas.o
obj-$(CONFIG_PREP_RESIDUAL)	+= residual.o
obj-$(CONFIG_ADIR)		+= adir_setup.o adir_pic.o adir_pci.o
obj-$(CONFIG_CPCI690)		+= cpci690.o
obj-$(CONFIG_EV64260)		+= ev64260.o
obj-$(CONFIG_GEMINI)		+= gemini_pci.o gemini_setup.o gemini_prom.o
obj-$(CONFIG_GLACIER)		+= glacier.o
obj-$(CONFIG_ICECUBE)		+= icecube.o
obj-$(CONFIG_K2)		+= k2_setup.o k2_pci.o
obj-$(CONFIG_LOPEC)		+= lopec_setup.o lopec_pci.o
obj-$(CONFIG_MCPN765)		+= mcpn765_setup.o mcpn765_pci.o
obj-$(CONFIG_MVME5100)		+= mvme5100_setup.o mvme5100_pci.o
obj-$(CONFIG_PAL4)		+= pal4_setup.o pal4_pci.o cpc700_pic.o
obj-$(CONFIG_PCORE)		+= pcore_setup.o pcore_pci.o
obj-$(CONFIG_POWERPMC250)	+= powerpmc250.o
obj-$(CONFIG_PPMC260)		+= ppmc260.o
obj-$(CONFIG_PPLUS)		+= pplus_pci.o pplus_setup.o
obj-$(CONFIG_PRPMC750)		+= prpmc750_setup.o prpmc750_pci.o
obj-$(CONFIG_PUMA_A)            += pumaA.o
obj-$(CONFIG_PRPMC800)		+= prpmc800_setup.o prpmc800_pci.o
obj-$(CONFIG_SANDPOINT)		+= sandpoint_setup.o sandpoint_pci.o
obj-$(CONFIG_SL8245)		+= sl8245_setup.o sl8245_pci.o
obj-$(CONFIG_CU824)		+= cu824_setup.o cu824_pci.o
obj-$(CONFIG_SPRUCE)		+= spruce_setup.o spruce_pci.o cpc700_pic.o
obj-$(CONFIG_HXEB100)		+= hxeb100.o

ifeq ($(CONFIG_SMP),y)
obj-$(CONFIG_ALL_PPC)		+= pmac_smp.o chrp_smp.o
endif

include $(TOPDIR)/Rules.make
