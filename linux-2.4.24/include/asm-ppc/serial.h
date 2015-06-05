/*
 * include/asm-ppc/serial.h
 */

#ifdef __KERNEL__
#ifndef __ASM_SERIAL_H__
#define __ASM_SERIAL_H__

#include <linux/config.h>

#if defined(CONFIG_EV64260)
#include <platforms/ev64260.h>
#elif defined(CONFIG_GEMINI)
#include <platforms/gemini_serial.h>
#elif defined(CONFIG_POWERPMC250)
#include <platforms/powerpmc250_serial.h>
#elif defined(CONFIG_LOPEC)
#include <platforms/lopec_serial.h>
#elif defined(CONFIG_MCPN765)
#include <platforms/mcpn765_serial.h>
#elif defined(CONFIG_MVME5100)
#include <platforms/mvme5100_serial.h>
#elif defined(CONFIG_PAL4)
#include <platforms/pal4_serial.h>
#elif defined(CONFIG_PRPMC750)
#include <platforms/prpmc750_serial.h>
#elif defined(CONFIG_PRPMC800)
#include <platforms/prpmc800_serial.h>
#elif defined(CONFIG_SANDPOINT)
#include <platforms/sandpoint_serial.h>
#elif defined(CONFIG_HXEB100)
#include <platforms/hxeb100.h>
#elif defined(CONFIG_SPRUCE)
#include <platforms/spruce.h>
#elif defined(CONFIG_4xx)
#include <asm/ibm4xx.h>
#elif defined(CONFIG_DAB4K)
#include <platforms/dab4k_serial.h>
#elif defined(CONFIG_EXBITGEN)
#include <platforms/exbitgen.h>
#elif defined(CONFIG_SL8245)
#include <platforms/sl8245_serial.h>
#elif defined(CONFIG_CU824)
#include <platforms/cu824_serial.h>
#elif defined(CONFIG_GLACIER)
#include <platforms/glacier.h>
#elif defined(CONFIG_ICECUBE)
#include <platforms/icecube.h>
#else

/*
 * XXX Assume for now it has PC-style ISA serial ports.
 * This is true for PReP and CHRP at least.
 */
#include <asm/pc_serial.h>
#include <asm/processor.h>

#if defined(CONFIG_MAC_SERIAL)
#define SERIAL_DEV_OFFSET	((_machine == _MACH_prep || _machine == _MACH_chrp) ? 0 : 2)
#endif

#endif /* !CONFIG_GEMINI and others */
#endif /* __ASM_SERIAL_H__ */
#endif /* __KERNEL__ */
