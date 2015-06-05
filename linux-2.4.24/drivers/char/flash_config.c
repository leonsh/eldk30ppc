/*
 * (C) Copyright 2000-2004
 * DENX Software Engineering
 * Wolfgang Denk, wd@denx.de
 * All rights reserved.
 *
 * $Date: 2005/05/08 12:47:22 $
 * $Revision: 1.3 $
 *
 * This file defines the partitioning of flash devices in separate
 * "partitions". A partition is represented by a minor device, and
 * all erase / read / write operations are restricted to that device.
 * A separate minor device is provided which allows to access the whole
 * flash device. The maximum number of partitions can be adjusted by
 * changing the definition of FLASH_PART_BITS in "include/linux/flash.h".
 * Using 3 bits of the minor number to identify the partition, you can
 * define up to 7 partitions per flash device, which results in a
 * setup like this:
 *
 *	---------------------------------------------------------------
 *	Major	Minor	Device Name	Description
 *	---------------------------------------------------------------
 *	 60	  0	/dev/flasha	All of first flash device
 *	---------------------------------------------------------------
 *	 60	  1	/dev/flasha1	Partition 1 on 1st flash device
 *	 60	  2	/dev/flasha2	Partition 2 on 1st flash device
 *	 60	  3	/dev/flasha3	Partition 3 on 1st flash device
 *	 60	  4	/dev/flasha4	Partition 4 on 1st flash device
 *	 60	  5	/dev/flasha5	Partition 5 on 1st flash device
 *	 60	  6	/dev/flasha6	Partition 6 on 1st flash device
 *	 60	  7	/dev/flasha7	Partition 7 on 1st flash device
 *	---------------------------------------------------------------
 *	 60	  8	/dev/flashb	All of second flash device
 *	---------------------------------------------------------------
 *	 60	  9	/dev/flashb1	Partition 1 on 2nd flash device
 *	 60	 10	/dev/flashb2	Partition 2 on 2nd flash device
 *	 60	 11	/dev/flashb3	Partition 3 on 2nd flash device
 *	 60	 12	/dev/flashb4	Partition 4 on 2nd flash device
 *	 60	 13	/dev/flashb5	Partition 5 on 2nd flash device
 *	 60	 14	/dev/flashb6	Partition 6 on 2nd flash device
 *	 60	 15	/dev/flashb7	Partition 7 on 2nd flash device
 *	---------------------------------------------------------------
 *	...
 *
 * Please be careful to align your partition definitions with the
 * sector organization of your flash chips, or strange errors will be
 * reported.
 *
 * You do not have to define all partitions for a device; you don't
 * have to define any partitions at all if you don't need them. Any
 * partition definition with a "size" of 0 is considered as
 * non-existent.
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/flash.h>

/***** Configuration for AMX860 boards	*****************************************/

#if defined(CONFIG_AMX860)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,   256 << 10, }, /*	0     -	 256 kB = 0.250 MB => Monitor	*/
    {	 256 << 10,   768 << 10, }, /* 256 kB -	  1  MB = 0.750 MB => Kernel	*/
    {	1024 << 10,  3072 << 10, }, /*	1  MB -	  4  MB =   3	MB => RAMDisk	*/
  },
  /*
   * Definitions for FLASH Device 1:
   */
  { /* Start offset   Size */
    {		 0,  4096 << 10, }, /*	 0     -   4 MB = 4.0 MB	*/
  },
};

/***** Configuration for C2MON boards *******************************************/

#elif defined(CONFIG_C2MON)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,   256 << 10, }, /*	0     -	 256 kB = 0.250 MB => Monitor	*/
    {	 256 << 10,   768 << 10, }, /* 256 kB -	  1  MB = 0.750 MB => Kernel	*/
    {	1024 << 10,  1536 << 10, }, /*	1  MB -	 2.5 MB =  1.5	MB => RAMDisk0	*/
    {	2560 << 10,  1536 << 10, }, /* 2.5 MB -	  4  MB =  1.5	MB => RAMDisk1	*/
  },
  /*
   * Definitions for FLASH Device 1:
   */
  { /* Start offset   Size */
    {		 0,  2048 << 10, }, /*	 0     -   2 MB = 2.0 MB	*/
    {	2048 << 10,  2048 << 10, }, /*	 2  MB -   4 MB = 2.0 MB	*/
  },
};

/***** Configuration for CU824 board  *******************************************/

#elif defined(CONFIG_CU824)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,  2048 << 10, }, /*	 0    -	   2 MB = 2.0 MB	*/
    {	2048 << 10,  2048 << 10, }, /*	 2 MB -	   4 MB = 2.0 MB	*/
    {	4096 << 10,  2048 << 10, }, /*	 4 MB -	   6 MB = 2.0 MB	*/
    {	6144 << 10,  2048 << 10, }, /*	 6 MB -	   8 MB = 2.0 MB	*/
  },
  /*
   * Definitions for FLASH Device 1:
   */
  { /* Start offset   Size */
    {		 0,  2048 << 10, }, /*	 0    -	   2 MB = 2.0 MB	*/
    {	2048 << 10,  2048 << 10, }, /*	 2 MB -	   4 MB = 2.0 MB	*/
    {	4096 << 10,  2048 << 10, }, /*	 4 MB -	   6 MB = 2.0 MB	*/
    {	6144 << 10,  1024 << 10, }, /*	 6 MB -	   7 MB = 1.0 MB	*/
    {	7168 << 10,   256 << 10, }, /*	 7 MB - 7.25 MB = 256 kB U-Boot */
    {	7424 << 10,   512 << 10, }, /*7.25 MB - 7.75 MB = 512 kB free	*/
    {	7936 << 10,   256 << 10, }, /*7.75 MB -	   8 MB = 256 kB Env.	*/
  },
};

/***** Configuration for HERMES-PRO  ********************************************/

#elif	defined(CONFIG_HERMES)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,   128 << 10, },	/*  0	  -  128 kB => U-Boot Monitor	*/
    {	 128 << 10,   896 << 10, },	/* 128 kB - 1024 kB => Linux Kernel	*/
    {	1024 << 10,  1024 << 10, },	/*  1  MB -   2	 MB => ramdisk image	*/
  },
};

/*****	Configuration for ICU862  ********************************************/

#elif defined(CONFIG_ICU862)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		  0,   4096 << 10, },	/*  0 -	 4 MB				*/
    {	 4096 << 10,   4096 << 10, },	/*  4 -	 8 MB				*/
    {	 8192 << 10,   4096 << 10, },	/*  8 - 12 MB				*/
    {	12288 << 10,   3072 << 10, },	/* 12 - 15 MB				*/
    {	15360 << 10,	256 << 10, },	/* 256 KB => PPCboot			*/
    {	15616 << 10,	256 << 10, },	/* 256 KB => NVRAM			*/
    {	15872 << 10,	512 << 10, },	/* 15.5 MB -  16  MB =	0.5  MB		*/
  },
};

/***** Configuration for DAB4K ***********************************************/

#elif defined(CONFIG_DAB4K)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
 {
   /*
    * Definitions for FLASH Device 0:
    */
   { /* Start offset   Size */
     {		0,   256 << 10, }, /*	 0 kB -	 256 kB => U-Boot Monitor	*/
     {	256 << 10,    64 << 10, }, /*  256 kB -	 320 kB => Environment		*/
     { 1024 << 10,  7168 << 10, }, /* 1024 kB - 8192 kB => Flashdisk		*/
   },
};

/***** Configuration for IP860 **************************************************/

#elif	defined(CONFIG_IP860)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,   128 << 10, },	/*   0	   -  128 kB => U-Boot Monitor	*/
    {	 128 << 10,   896 << 10, },	/*  128 kB - 1024 kB => Linux Kernel	*/
    {	1024 << 10,  1024 << 10, },	/* 1024 kB - 2048 kB =>			*/
    {	2048 << 10,  1024 << 10, },	/* 2048 kB - 3072 kB =>			*/
    {	3072 << 10,  1024 << 10, },	/* 3072 kB - 4096 kB =>			*/
  },
};

/***** Configuration for IVMS8 and IVML24 ***************************************/

#elif	defined(CONFIG_IVMS8) || defined(CONFIG_IVML24)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,   256 << 10, },	/*  0	  -  256 kB => U-Boot Monitor	*/
    {	 256 << 10,   232 << 10, },	/* 256 kB -  488 kB => unused		*/
    {	 488 << 10,	8 << 10, },	/* 488 kB -  496 kB => Environment	*/
    {	 496 << 10,    16 << 10, },	/* 496 kB -  512 kB => unused		*/
  },
};

/***** Configuration for LANTEC boards	*****************************************/

#elif defined(CONFIG_LANTEC)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,   256 << 10, }, /*	0     -	 256 kB = 0.250 MB => Monitor	*/
    {	 256 << 10,   768 << 10, }, /* 256 kB -	  1  MB = 0.750 MB => Kernel	*/
    {	1024 << 10,  1536 << 10, }, /*	1  MB -	 2.5 MB =  1.5	MB => RAMDisk0	*/
    {	2560 << 10,  1536 << 10, }, /* 2.5 MB -	  4  MB =  1.5	MB => RAMDisk1	*/
  },
  /*
   * Definitions for FLASH Device 1:
   */
  { /* Start offset   Size */
    {		 0,  2048 << 10, }, /*	 0     -   2 MB = 2.0 MB	*/
    {	2048 << 10,  2048 << 10, }, /*	 2  MB -   4 MB = 2.0 MB	*/
  },
};

/***** Configuration for LWMON	*************************************************/

#elif	defined(CONFIG_LWMON)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,   256 << 10, },	/*  0	  -  256 kB => U-Boot Monitor	*/
    {	 256 << 10,   768 << 10, },	/* 256 kB - 1024 kB => Linux   Kernel	*/
    {	1024 << 10,  1024 << 10, },	/*  1  MB -   2	 MB => altern. Kernel	*/
    {	2048 << 10,  3072 << 10, },	/*  2  MB -   5	 MB => ramdisk image	*/
    {	5120 << 10,  3072 << 10, },	/*  5  MB -   8	 MB => ramdisk image	*/
  },
  /*
   * Definitions for FLASH Device 1:
   */
  { /* Start offset   Size */
    {		 0,  4096 << 10, },	/*  0	  -   4	 MB			*/
    {	4096 << 10,  4096 << 10, },	/*  4  MB -   8	 MB			*/
  },
};

/***** Configuration for the PM826 module ***************************************/

#elif defined(CONFIG_PM826)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,   256 << 10, }, /*	0     -	 256 kB => Monitor	*/
    {	 256 << 10,   768 << 10, }, /* 256 kB -	  1  MB => Kernel	*/
    {	1024 << 10,  1536 << 10, }, /*	1  MB -	 2.5 MB => RAMDisk0	*/
    {	2560 << 10,  1536 << 10, }, /* 2.5 MB -	  4  MB => RAMDisk1	*/
  },
};

/***** Configuration for PCU E	*************************************************/

#elif	defined(CONFIG_PCU_E)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,  1024 << 10, },	/*  0	  -   1	 MB =>			*/
    {	1024 << 10,  1024 << 10, },	/*  1  MB -   2	 MB =>			*/
    {	2048 << 10,  1024 << 10, },	/*  2  MB -   3	 MB =>			*/
    {	3072 << 10,  1024 << 10, },	/*  3  MB -   4	 MB =>			*/
  },
  /*
   * Definitions for FLASH Device 1:
   */
  { /* Start offset   Size */
    {		 0,  1024 << 10, },	/*  0	  -   1	 MB =>			*/
    {	1024 << 10,  1024 << 10, },	/*  1  MB -   2	 MB =>			*/
    {	2048 << 10,  1024 << 10, },	/*  2  MB -   3	 MB =>			*/
    {	3072 << 10,  1000 << 10, },	/*  3  MB -  (4) MB => U-Boot Monitor	*/
    {	4072 << 10,	8 << 10, },	/* 3rd last 8k "boot" sector		*/
    {	4080 << 10,	8 << 10, },	/* 2nd last 8k "boot" sector		*/
    {	4088 << 10,	8 << 10, },	/* last "boot" sector => Environment	*/
  },
};

/***** Configuration for RedRock vision boards **********************************/

#elif defined(CONFIG_RRVISION)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,   256 << 10, }, /*	0     -	 256 kB = 0.250 MB => Monitor	*/
    {	 256 << 10,   768 << 10, }, /* 256 kB -	  1  MB = 0.750 MB => Kernel	*/
    {	1024 << 10,  1536 << 10, }, /*	1  MB -	 2.5 MB =  1.5	MB => RAMDisk0	*/
    {	2560 << 10,  1536 << 10, }, /* 2.5 MB -	  4  MB =  1.5	MB => RAMDisk1	*/
    {	4096 << 10,  4096 << 10, }, /*	4  MB -	  8  MB =  4.0	MB */
    {	8192 << 10,  8192 << 10, }, /*	8  MB -	 16  MB =  8.0	MB */
  },
};


/***** Configuration for TQM8xxL modules ****************************************/

#elif defined(CONFIG_TQM8xxL) && !defined(CONFIG_TQM8xxM)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,   256 << 10, }, /*	0     -	 256 kB = 0.250 MB => Monitor	*/
    {	 256 << 10,   768 << 10, }, /* 256 kB -	  1  MB = 0.750 MB => Kernel	*/
    {	1024 << 10,  1536 << 10, }, /*	1  MB -	 2.5 MB =  1.5	MB => RAMDisk0	*/
    {	2560 << 10,  1536 << 10, }, /* 2.5 MB -	  4  MB =  1.5	MB => RAMDisk1	*/
  },
  /*
   * Definitions for FLASH Device 1:
   */
  { /* Start offset   Size */
    {		 0,  2048 << 10, }, /*	 0     -   2 MB = 2.0 MB	*/
    {	2048 << 10,  2048 << 10, }, /*	 2  MB -   4 MB = 2.0 MB	*/
  },
};

/***** Configuration for TQM8xxM modules ****************************************/

#elif defined(CONFIG_TQM8xxM)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,   512 << 10, }, /*	0     -	 512 kB =  0.5 MB => Monitor	*/
    {	 512 << 10,  1024 << 10, }, /* 0.5 MB -	 1.5 MB =  1.5 MB => Kernel	*/
    {	1536 << 10,  2560 << 10, }, /* 1.5 MB -	 4.0 MB =  2.5 MB => RAMDisk0	*/
    {	2560 << 10,  1536 << 10, }, /* 2.5 MB -	  4  MB =  1.5 MB => RAMDisk1	*/
    {	   4 << 20,	4 << 20, }, /*	 4     -   8 MB =  4.0 MB	*/
    {	   8 << 20,	4 << 20, }, /*	 8     -  12 MB =  4.0 MB	*/
    {	  12 << 20,	4 << 20, }, /*	12     -  16 MB =  4.0 MB	*/
    {	  16 << 20,    16 << 20, }, /*	16     -  32 MB = 16.0 MB	*/
  },
};

/***** Configuration for SIEMENS CCM modules ************************************/

#elif defined(CONFIG_CCM)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,   256 << 10, }, /*	0     -	 256 kB = 0.25 MB => ppcBoot	    */
    {	 256 << 10,   256 << 10, }, /* 256 kB -	 512 kB = 0.25 MB => PUMA Netlist   */
    {	 512 << 10,  1024 << 10, }, /* 512 kB -	 1.5 MB = 1.00 MB => CRAMFS	    */
    {	1536 << 10,  2560 << 10, }, /* 1.5 MB -	  4  MB = 2.50 MB => Kernel/RAMDisk */
  },
  /*
   * Definitions for FLASH Device 1:
   */
  { /* Start offset   Size */
    {		 0,  1536 << 10, }, /*	  0   -	 1.5 MB = 1.5 MB  => JFFS	    */
    {	1536 << 10,  2560 << 10, }, /* 1.5 MB -	 4.0 MB = 2.5 MB  => VCDB/Kernel2   */
  },
};

/***** Configuration for the TQM8260 module ************************************/

#elif defined(CONFIG_TQM8260)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,   256 << 10, }, /*	0     -	 256 kB => Monitor	*/
    {	 256 << 10,   768 << 10, }, /* 256 kB -	  1  MB => Kernel	*/
    {	   1 << 20,	3 << 20, }, /*	1  MB -	  4  MB => RAMDisk	*/
    {	   4 << 20,	4 << 20, }, /*	4  MB -	  8  MB => JFFS2 FS	*/
  },
};

// hwuang, 10/28/04
#elif defined(CONFIG_OLC8260)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,   256 << 10, }, /*	0     -	 256 kB => Monitor	*/
    {	 256 << 10,   768 << 10, }, /* 256 kB -	  1  MB => Kernel	*/
    {	   1 << 20,	8 << 20, }, /*	1  MB -	  9  MB => RAMDisk	*/
    {	   9 << 20,	7 << 20, }, /*	9  MB -	  16 MB => JFFS2 FS	*/
  },
  /*
   * Definitions for FLASH Device 1:
   * JieHan 07/27/2005 
   */
  { /* Start offset   Size */
    {	128 << 10,   ((14 << 20) + (896 << 10)), }, /*	128KB    -	 15 MB => JFFS2 FS	*/
  },
};

// hwuang, 12/22/04
#elif defined(CONFIG_ONU8260)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,   256 << 10, }, /*	0     -	 256 kB => Monitor	*/
    {	 256 << 10,   768 << 10, }, /* 256 kB -	  1  MB => Kernel	*/
    {	   1 << 20,	8 << 20, }, /*	1  MB -	  9  MB => RAMDisk	*/
    {	   9 << 20,	6 << 20, }, /*	9  MB -	  15  MB => JFFS2 FS	*/
  },
};

/***** Configuration for the PCIPPC-2 / PCIPPC-6 module ************************/

#elif defined(CONFIG_PCIPPC)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,   256 << 10, }, /*	0     -	 256 kB => U-Boot Monitor  */
    {	 256 << 10,   192 << 10, }, /* 256 kB -	 448 kB => unused	   */
    {	 448 << 10,    64 << 10, }, /* 448 kB -	 512 kB => Environment	   */
  },
};

/***** Configuration for the IPHASE4539 module *********************************/

#elif defined(CONFIG_IPHASE4539)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,    64 << 10, }, /*	  0    -   64 kB => HCW		*/
    {	  64 << 10,   128 << 10, }, /*	 64 kB -  128 kB => Environment */
    {	 128 << 10,  3072 << 10, }, /*	128 kB -    3 MB => RAMDisk	*/
    {	3072 << 10,  3328 << 10, }, /*	  3 MB - 3328 KB => PPCboot	*/
    {	3328 << 10,  4096 << 10, }, /* 3328 KB -    4 MB => Kernel	*/
  },
};

/***** Configuration for the CPU86 module ************************************/

#elif defined(CONFIG_CPU86)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,   192 << 10, }, /*	0     -	 192 kB => U-Boot	*/
    {	 192 << 10,   512 << 10, }, /* 192 kB -	 512 kB => Free		*/
  },
};

/***** Configuration for the R360MPI Board ************************************/

#elif defined(CONFIG_R360MPI)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Definitions for FLASH Device 0:
   */
  { /* Start offset   Size */
    {		 0,   256 << 10, }, /*	0     -	 256 kB => U-Boot	*/
    {	 128 << 10,  8192 << 10, }, /* 256 kB - 8192 kB => Free		*/
  },
};

/***** Configuration for FADS board ****************************************/

#elif defined(CONFIG_FADS860)
flash_partition_t flash_partition_table[][FLASH_PART_MAX-1] =
{
  /*
   * Flash Device 0 (2 MB)
   */
  { /* Start offset   Size */
    {		 0,   256 << 10, },	/*   0	   -  256 kB => U-Boot Monitor	*/
    {	 256 << 10,   768 << 10, },	/*  256 kB - 1024 kB => Linux Kernel	*/
    {	1024 << 10,  768 << 10, },	/* 1024 kB - 1792 kB => Ramdisk		*/
    {	1792 << 10,  256 << 10, },	/* 1792 kB - 2048 kB => Environment	*/
  },

};

/***** Default Configuration  ***************************************************/

#else
# error Flash Partition Table not configured
#endif
/********************************************************************************/

int flash_part_num = sizeof(flash_partition_table)
		   / sizeof(flash_partition_t[FLASH_PART_MAX-1]);
