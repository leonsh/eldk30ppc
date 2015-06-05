/*
 * Automatically generated C config: don't edit
 */
#define AUTOCONF_INCLUDED
#undef  CONFIG_UID16
#undef  CONFIG_RWSEM_GENERIC_SPINLOCK
#define CONFIG_RWSEM_XCHGADD_ALGORITHM 1
#define CONFIG_HAVE_DEC_LOCK 1

/*
 * Code maturity level options
 */
#define CONFIG_EXPERIMENTAL 1
#define CONFIG_ADVANCED_OPTIONS 1

/*
 * Loadable module support
 */
#define CONFIG_MODULES 1
#undef  CONFIG_MODVERSIONS
#define CONFIG_KMOD 1

/*
 * Platform support
 */
#define CONFIG_PPC 1
#define CONFIG_PPC32 1
#undef  CONFIG_6xx
#undef  CONFIG_40x
#undef  CONFIG_44x
#undef  CONFIG_POWER3
#undef  CONFIG_POWER4
#define CONFIG_8xx 1
#undef  CONFIG_PPC_STD_MMU
#define CONFIG_SERIAL_CONSOLE 1
#define CONFIG_NOT_COHERENT_CACHE 1
#undef  CONFIG_ADDER_II
#undef  CONFIG_AMX860
#undef  CONFIG_BSEIP
#undef  CONFIG_CCM
#undef  CONFIG_DBOX2
#undef  CONFIG_DAB4K
#undef  CONFIG_FADS
#undef  CONFIG_FPS850L
#undef  CONFIG_HERMES_PRO
#undef  CONFIG_IP860
#undef  CONFIG_IVML24
#undef  CONFIG_IVMS8
#undef  CONFIG_LANTEC
#undef  CONFIG_LWMON
#undef  CONFIG_MBX
#define CONFIG_NSCU 1
#undef  CONFIG_PCU_E
#undef  CONFIG_RMU
#undef  CONFIG_RPXCLASSIC
#undef  CONFIG_RPXLITE
#undef  CONFIG_SM850
#undef  CONFIG_SPD823TS
#undef  CONFIG_TQM823L
#undef  CONFIG_TQM823M
#undef  CONFIG_TQM850L
#undef  CONFIG_TQM850M
#undef  CONFIG_TQM855L
#undef  CONFIG_TQM855M
#undef  CONFIG_TQM860M
#undef  CONFIG_TQM862M
#undef  CONFIG_TQM860L
#undef  CONFIG_VIPER860
#undef  CONFIG_WINCEPT
#define CONFIG_TQM8xxM 1
#define CONFIG_TQM8xxL 1
#undef  CONFIG_ALL_PPC
#undef  CONFIG_SMP
#undef  CONFIG_MATH_EMULATION
#define CONFIG_EMBEDDEDBOOT 1

/*
 * General setup
 */
#undef  CONFIG_HIGHMEM
#undef  CONFIG_LOWMEM_SIZE_BOOL
#undef  CONFIG_KERNEL_START_BOOL
#undef  CONFIG_TASK_SIZE_BOOL
#undef  CONFIG_PIN_TLB
#define CONFIG_HIGHMEM_START 0xfe000000
#define CONFIG_LOWMEM_SIZE 0x30000000
#define CONFIG_KERNEL_START 0xc0000000
#define CONFIG_TASK_SIZE 0x80000000
#undef  CONFIG_ISA
#undef  CONFIG_EISA
#undef  CONFIG_SBUS
#undef  CONFIG_MCA
#undef  CONFIG_PCI_QSPAN
#undef  CONFIG_PCI
#define CONFIG_NET 1
#define CONFIG_SYSCTL 1
#define CONFIG_SYSVIPC 1
#undef  CONFIG_BSD_PROCESS_ACCT
#define CONFIG_KCORE_ELF 1
#define CONFIG_BINFMT_ELF 1
#define CONFIG_KERNEL_ELF 1
#undef  CONFIG_BINFMT_MISC
#define CONFIG_HOTPLUG 1

/*
 * PCMCIA/CardBus support
 */
#undef  CONFIG_PCMCIA
#define CONFIG_PCMCIA_MODULE 1
#undef  CONFIG_TCIC
#undef  CONFIG_I82092
#undef  CONFIG_I82365
#undef  CONFIG_PCMCIA_M8XX
#define CONFIG_PCMCIA_M8XX_MODULE 1

/*
 * Parallel port support
 */
#undef  CONFIG_PARPORT
#undef  CONFIG_GEN_RTC
#undef  CONFIG_PPC_RTC
#undef  CONFIG_CMDLINE_BOOL

/*
 * Memory Technology Devices (MTD)
 */
#define CONFIG_MTD 1
#undef  CONFIG_MTD_DEBUG
#define CONFIG_MTD_PARTITIONS 1
#undef  CONFIG_MTD_CONCAT
#undef  CONFIG_MTD_REDBOOT_PARTS
#undef  CONFIG_MTD_CMDLINE_PARTS

/*
 * User Modules And Translation Layers
 */
#define CONFIG_MTD_CHAR 1
#define CONFIG_MTD_BLOCK 1
#undef  CONFIG_FTL
#undef  CONFIG_NFTL

/*
 * RAM/ROM/Flash chip drivers
 */
#define CONFIG_MTD_CFI 1
#undef  CONFIG_MTD_JEDECPROBE
#define CONFIG_MTD_GEN_PROBE 1
#undef  CONFIG_MTD_CFI_ADV_OPTIONS
#undef  CONFIG_MTD_CFI_INTELEXT
#define CONFIG_MTD_CFI_AMDSTD 1
#undef  CONFIG_MTD_CFI_STAA
#undef  CONFIG_MTD_RAM
#define CONFIG_MTD_ROM 1
#undef  CONFIG_MTD_ABSENT
#undef  CONFIG_MTD_OBSOLETE_CHIPS
#undef  CONFIG_MTD_AMDSTD
#undef  CONFIG_MTD_SHARP
#undef  CONFIG_MTD_JEDEC

/*
 * Mapping drivers for chip access
 */
#undef  CONFIG_MTD_PHYSMAP
#define CONFIG_MTD_TQM8XXM 1
#undef  CONFIG_MTD_MBX860
#undef  CONFIG_MTD_DBOX2
#undef  CONFIG_MTD_CFI_FLAGADM
#undef  CONFIG_MTD_AM8XX
#undef  CONFIG_MTD_DAB4K
#undef  CONFIG_MTD_AMX860
#undef  CONFIG_MTD_PCI
#undef  CONFIG_MTD_PCMCIA

/*
 * Self-contained MTD device drivers
 */
#undef  CONFIG_MTD_PMC551
#undef  CONFIG_MTD_SLRAM
#undef  CONFIG_MTD_MTDRAM
#undef  CONFIG_MTD_BLKMTD

/*
 * Disk-On-Chip Device Drivers
 */
#undef  CONFIG_MTD_DOC1000
#undef  CONFIG_MTD_DOC2000
#undef  CONFIG_MTD_DOC2001
#undef  CONFIG_MTD_DOCPROBE

/*
 * NAND Flash Device Drivers
 */
#undef  CONFIG_MTD_NAND

/*
 * Plug and Play configuration
 */
#undef  CONFIG_PNP
#undef  CONFIG_ISAPNP

/*
 * Block devices
 */
#undef  CONFIG_BLK_DEV_FD
#undef  CONFIG_BLK_DEV_XD
#undef  CONFIG_PARIDE
#undef  CONFIG_BLK_CPQ_DA
#undef  CONFIG_BLK_CPQ_CISS_DA
#undef  CONFIG_CISS_SCSI_TAPE
#undef  CONFIG_CISS_MONITOR_THREAD
#undef  CONFIG_BLK_DEV_DAC960
#undef  CONFIG_BLK_DEV_UMEM
#undef  CONFIG_BLK_DEV_LOOP
#undef  CONFIG_BLK_DEV_NBD
#define CONFIG_BLK_DEV_RAM 1
#define CONFIG_BLK_DEV_RAM_SIZE (4096)
#define CONFIG_BLK_DEV_INITRD 1
#undef  CONFIG_BLK_STATS

/*
 * Multi-device support (RAID and LVM)
 */
#undef  CONFIG_MD
#undef  CONFIG_BLK_DEV_MD
#undef  CONFIG_MD_LINEAR
#undef  CONFIG_MD_RAID0
#undef  CONFIG_MD_RAID1
#undef  CONFIG_MD_RAID5
#undef  CONFIG_MD_MULTIPATH
#undef  CONFIG_BLK_DEV_LVM

/*
 * Networking options
 */
#define CONFIG_PACKET 1
#undef  CONFIG_PACKET_MMAP
#define CONFIG_NETLINK_DEV 1
#undef  CONFIG_NETFILTER
#undef  CONFIG_FILTER
#define CONFIG_UNIX 1
#define CONFIG_INET 1
#undef  CONFIG_IP_MULTICAST
#undef  CONFIG_IP_ADVANCED_ROUTER
#define CONFIG_IP_PNP 1
#undef  CONFIG_IP_PNP_DHCP
#undef  CONFIG_IP_PNP_BOOTP
#undef  CONFIG_IP_PNP_RARP
#undef  CONFIG_NET_IPIP
#undef  CONFIG_NET_IPGRE
#undef  CONFIG_ARPD
#undef  CONFIG_INET_ECN
#undef  CONFIG_SYN_COOKIES
#undef  CONFIG_IPV6
#undef  CONFIG_KHTTPD

/*
 *    SCTP Configuration (EXPERIMENTAL)
 */
#define CONFIG_IPV6_SCTP__ 1
#undef  CONFIG_IP_SCTP
#undef  CONFIG_ATM
#undef  CONFIG_VLAN_8021Q

/*
 *  
 */
#undef  CONFIG_IPX
#undef  CONFIG_ATALK

/*
 * Appletalk devices
 */
#undef  CONFIG_DEV_APPLETALK
#undef  CONFIG_DECNET
#undef  CONFIG_BRIDGE
#undef  CONFIG_X25
#undef  CONFIG_LAPB
#undef  CONFIG_LLC
#undef  CONFIG_NET_DIVERT
#undef  CONFIG_ECONET
#undef  CONFIG_WAN_ROUTER
#undef  CONFIG_NET_FASTROUTE
#undef  CONFIG_NET_HW_FLOWCONTROL

/*
 * QoS and/or fair queueing
 */
#undef  CONFIG_NET_SCHED

/*
 * Network testing
 */
#undef  CONFIG_NET_PKTGEN

/*
 * ATA/IDE/MFM/RLL support
 */
#define CONFIG_IDE 1

/*
 * IDE, ATA and ATAPI Block devices
 */
#define CONFIG_BLK_DEV_IDE 1

/*
 * Please see Documentation/ide.txt for help/info on IDE drives
 */
#undef  CONFIG_BLK_DEV_HD_IDE
#undef  CONFIG_BLK_DEV_HD
#define CONFIG_BLK_DEV_IDEDISK 1
#define CONFIG_IDEDISK_MULTI_MODE 1
#undef  CONFIG_IDEDISK_STROKE
#undef  CONFIG_BLK_DEV_IDECS
#define CONFIG_BLK_DEV_IDECS_MODULE 1
#undef  CONFIG_BLK_DEV_IDECD
#undef  CONFIG_BLK_DEV_IDETAPE
#undef  CONFIG_BLK_DEV_IDEFLOPPY
#undef  CONFIG_BLK_DEV_IDESCSI
#undef  CONFIG_IDE_TASK_IOCTL

/*
 * IDE chipset support/bugfixes
 */
#undef  CONFIG_BLK_DEV_CMD640
#undef  CONFIG_BLK_DEV_CMD640_ENHANCED
#undef  CONFIG_BLK_DEV_ISAPNP
#undef  CONFIG_BLK_DEV_MPC8xx_IDE
#undef  CONFIG_IDE_CHIPSETS
#undef  CONFIG_IDEDMA_AUTO
#undef  CONFIG_DMA_NONPCI
#define CONFIG_BLK_DEV_IDE_MODES 1
#undef  CONFIG_BLK_DEV_ATARAID
#undef  CONFIG_BLK_DEV_ATARAID_PDC
#undef  CONFIG_BLK_DEV_ATARAID_HPT
#undef  CONFIG_BLK_DEV_ATARAID_SII

/*
 * SCSI support
 */
#undef  CONFIG_SCSI

/*
 * Fusion MPT device support
 */
#undef  CONFIG_FUSION
#undef  CONFIG_FUSION_BOOT
#undef  CONFIG_FUSION_ISENSE
#undef  CONFIG_FUSION_CTL
#undef  CONFIG_FUSION_LAN

/*
 * I2O device support
 */
#undef  CONFIG_I2O
#undef  CONFIG_I2O_BLOCK
#undef  CONFIG_I2O_LAN
#undef  CONFIG_I2O_SCSI
#undef  CONFIG_I2O_PROC

/*
 * Network device support
 */
#define CONFIG_NETDEVICES 1

/*
 * ARCnet devices
 */
#undef  CONFIG_ARCNET
#undef  CONFIG_DUMMY
#undef  CONFIG_BONDING
#undef  CONFIG_EQUALIZER
#undef  CONFIG_TUN
#undef  CONFIG_ETHERTAP

/*
 * Ethernet (10 or 100Mbit)
 */
#define CONFIG_NET_ETHERNET 1
#undef  CONFIG_MACE
#undef  CONFIG_BMAC
#undef  CONFIG_GMAC
#undef  CONFIG_SUNLANCE
#undef  CONFIG_SUNBMAC
#undef  CONFIG_SUNQE
#undef  CONFIG_SUNGEM
#undef  CONFIG_NET_VENDOR_3COM
#undef  CONFIG_LANCE
#undef  CONFIG_NET_VENDOR_SMC
#undef  CONFIG_NET_VENDOR_RACAL
#undef  CONFIG_NET_ISA
#undef  CONFIG_NET_PCI
#undef  CONFIG_NET_POCKET

/*
 * Ethernet (1000 Mbit)
 */
#undef  CONFIG_ACENIC
#undef  CONFIG_DL2K
#undef  CONFIG_E1000
#undef  CONFIG_MYRI_SBUS
#undef  CONFIG_NS83820
#undef  CONFIG_HAMACHI
#undef  CONFIG_YELLOWFIN
#undef  CONFIG_R8169
#undef  CONFIG_SK98LIN
#undef  CONFIG_TIGON3
#undef  CONFIG_FDDI
#undef  CONFIG_HIPPI
#undef  CONFIG_PLIP
#undef  CONFIG_PPP
#undef  CONFIG_SLIP

/*
 * Wireless LAN (non-hamradio)
 */
#undef  CONFIG_NET_RADIO

/*
 * Token Ring devices
 */
#undef  CONFIG_TR
#undef  CONFIG_NET_FC
#undef  CONFIG_RCPCI
#undef  CONFIG_SHAPER

/*
 * Wan interfaces
 */
#undef  CONFIG_WAN

/*
 * PCMCIA network device support
 */
#undef  CONFIG_NET_PCMCIA

/*
 * Amateur Radio support
 */
#undef  CONFIG_HAMRADIO

/*
 * IrDA (infrared) support
 */
#undef  CONFIG_IRDA

/*
 * ISDN subsystem
 */
#undef  CONFIG_ISDN

/*
 * Old CD-ROM drivers (not SCSI, not IDE)
 */
#undef  CONFIG_CD_NO_IDESCSI

/*
 * Console drivers
 */

/*
 * Frame-buffer support
 */
#undef  CONFIG_FB

/*
 * Input core support
 */
#undef  CONFIG_INPUT
#undef  CONFIG_INPUT_KEYBDEV
#undef  CONFIG_INPUT_MOUSEDEV
#undef  CONFIG_INPUT_JOYDEV
#undef  CONFIG_INPUT_EVDEV
#undef  CONFIG_INPUT_UINPUT

/*
 * Macintosh device drivers
 */

/*
 * Character devices
 */
#undef  CONFIG_VT
#undef  CONFIG_SERIAL
#undef  CONFIG_SERIAL_EXTENDED
#undef  CONFIG_SERIAL_NONSTANDARD
#define CONFIG_UNIX98_PTYS 1
#define CONFIG_UNIX98_PTY_COUNT (32)

/*
 * I2C support
 */
#undef  CONFIG_I2C

/*
 * Mice
 */
#undef  CONFIG_BUSMOUSE
#undef  CONFIG_MOUSE

/*
 * Joysticks
 */
#undef  CONFIG_INPUT_GAMEPORT

/*
 * Input core support is needed for gameports
 */

/*
 * Input core support is needed for joysticks
 */
#undef  CONFIG_QIC02_TAPE
#undef  CONFIG_IPMI_HANDLER
#undef  CONFIG_IPMI_PANIC_EVENT
#undef  CONFIG_IPMI_DEVICE_INTERFACE
#undef  CONFIG_IPMI_KCS
#undef  CONFIG_IPMI_WATCHDOG

/*
 * Watchdog Cards
 */
#undef  CONFIG_WATCHDOG
#undef  CONFIG_SCx200_GPIO
#undef  CONFIG_AMD_PM768
#undef  CONFIG_NVRAM
#undef  CONFIG_RTC
#undef  CONFIG_RTC_11_MINUTE_MODE
#undef  CONFIG_PCF8563_RTC
#define CONFIG_RTC_8XX 1
#undef  CONFIG_DTLK
#undef  CONFIG_R3964
#undef  CONFIG_APPLICOM
#undef  CONFIG_FLASH
#define CONFIG_STATUS_LED 1

/*
 * Ftape, the floppy tape device driver
 */
#undef  CONFIG_FTAPE
#undef  CONFIG_AGP

/*
 * Direct Rendering Manager (XFree86 DRI support)
 */
#undef  CONFIG_DRM

/*
 * PCMCIA character devices
 */
#undef  CONFIG_PCMCIA_SERIAL_CS
#undef  CONFIG_SYNCLINK_CS

/*
 * Multimedia devices
 */
#undef  CONFIG_VIDEO_DEV

/*
 * File systems
 */
#undef  CONFIG_QUOTA
#undef  CONFIG_QFMT_V2
#undef  CONFIG_AUTOFS_FS
#undef  CONFIG_AUTOFS4_FS
#undef  CONFIG_REISERFS_FS
#undef  CONFIG_REISERFS_CHECK
#undef  CONFIG_REISERFS_PROC_INFO
#undef  CONFIG_ADFS_FS
#undef  CONFIG_ADFS_FS_RW
#undef  CONFIG_AFFS_FS
#undef  CONFIG_HFS_FS
#undef  CONFIG_HFSPLUS_FS
#undef  CONFIG_BEFS_FS
#undef  CONFIG_BEFS_DEBUG
#undef  CONFIG_BFS_FS
#undef  CONFIG_EXT3_FS
#undef  CONFIG_JBD
#undef  CONFIG_JBD_DEBUG
#define CONFIG_FAT_FS 1
#define CONFIG_MSDOS_FS 1
#undef  CONFIG_UMSDOS_FS
#undef  CONFIG_VFAT_FS
#undef  CONFIG_EFS_FS
#define CONFIG_JFFS_FS 1
#define CONFIG_JFFS_FS_VERBOSE (0)
#undef  CONFIG_JFFS_PROC_FS
#define CONFIG_JFFS2_FS 1
#define CONFIG_JFFS2_FS_DEBUG (0)
#undef  CONFIG_JFFS2_FS_NAND
#define CONFIG_CRAMFS 1
#define CONFIG_TMPFS 1
#define CONFIG_RAMFS 1
#undef  CONFIG_ISO9660_FS
#undef  CONFIG_JOLIET
#undef  CONFIG_ZISOFS
#undef  CONFIG_JFS_FS
#undef  CONFIG_JFS_DEBUG
#undef  CONFIG_JFS_STATISTICS
#undef  CONFIG_MINIX_FS
#undef  CONFIG_VXFS_FS
#undef  CONFIG_NTFS_FS
#undef  CONFIG_NTFS_RW
#undef  CONFIG_HPFS_FS
#define CONFIG_PROC_FS 1
#undef  CONFIG_DEVFS_FS
#undef  CONFIG_DEVFS_MOUNT
#undef  CONFIG_DEVFS_DEBUG
#define CONFIG_DEVPTS_FS 1
#undef  CONFIG_QNX4FS_FS
#undef  CONFIG_QNX4FS_RW
#undef  CONFIG_ROMFS_FS
#define CONFIG_EXT2_FS 1
#undef  CONFIG_SYSV_FS
#undef  CONFIG_UDF_FS
#undef  CONFIG_UDF_RW
#undef  CONFIG_UFS_FS
#undef  CONFIG_UFS_FS_WRITE
#undef  CONFIG_XFS_FS
#undef  CONFIG_XFS_QUOTA
#undef  CONFIG_XFS_RT
#undef  CONFIG_XFS_TRACE
#undef  CONFIG_XFS_DEBUG

/*
 * Network File Systems
 */
#undef  CONFIG_CODA_FS
#undef  CONFIG_INTERMEZZO_FS
#define CONFIG_NFS_FS 1
#define CONFIG_NFS_V3 1
#undef  CONFIG_NFS_DIRECTIO
#define CONFIG_ROOT_NFS 1
#undef  CONFIG_NFSD
#undef  CONFIG_NFSD_V3
#undef  CONFIG_NFSD_TCP
#define CONFIG_SUNRPC 1
#define CONFIG_LOCKD 1
#define CONFIG_LOCKD_V4 1
#undef  CONFIG_SMB_FS
#undef  CONFIG_NCP_FS
#undef  CONFIG_NCPFS_PACKET_SIGNING
#undef  CONFIG_NCPFS_IOCTL_LOCKING
#undef  CONFIG_NCPFS_STRONG
#undef  CONFIG_NCPFS_NFS_NS
#undef  CONFIG_NCPFS_OS2_NS
#undef  CONFIG_NCPFS_SMALLDOS
#undef  CONFIG_NCPFS_NLS
#undef  CONFIG_NCPFS_EXTRAS
#undef  CONFIG_ZISOFS_FS

/*
 * Partition Types
 */
#define CONFIG_PARTITION_ADVANCED 1
#undef  CONFIG_ACORN_PARTITION
#undef  CONFIG_OSF_PARTITION
#undef  CONFIG_AMIGA_PARTITION
#undef  CONFIG_ATARI_PARTITION
#define CONFIG_MAC_PARTITION 1
#define CONFIG_MSDOS_PARTITION 1
#undef  CONFIG_BSD_DISKLABEL
#undef  CONFIG_MINIX_SUBPARTITION
#undef  CONFIG_SOLARIS_X86_PARTITION
#undef  CONFIG_UNIXWARE_DISKLABEL
#undef  CONFIG_LDM_PARTITION
#undef  CONFIG_SGI_PARTITION
#undef  CONFIG_ULTRIX_PARTITION
#undef  CONFIG_SUN_PARTITION
#undef  CONFIG_EFI_PARTITION
#undef  CONFIG_SMB_NLS
#define CONFIG_NLS 1

/*
 * Native Language Support
 */
#define CONFIG_NLS_DEFAULT "y"
#undef  CONFIG_NLS_CODEPAGE_437
#undef  CONFIG_NLS_CODEPAGE_737
#undef  CONFIG_NLS_CODEPAGE_775
#undef  CONFIG_NLS_CODEPAGE_850
#undef  CONFIG_NLS_CODEPAGE_852
#undef  CONFIG_NLS_CODEPAGE_855
#undef  CONFIG_NLS_CODEPAGE_857
#undef  CONFIG_NLS_CODEPAGE_860
#undef  CONFIG_NLS_CODEPAGE_861
#undef  CONFIG_NLS_CODEPAGE_862
#undef  CONFIG_NLS_CODEPAGE_863
#undef  CONFIG_NLS_CODEPAGE_864
#undef  CONFIG_NLS_CODEPAGE_865
#undef  CONFIG_NLS_CODEPAGE_866
#undef  CONFIG_NLS_CODEPAGE_869
#undef  CONFIG_NLS_CODEPAGE_936
#undef  CONFIG_NLS_CODEPAGE_950
#undef  CONFIG_NLS_CODEPAGE_932
#undef  CONFIG_NLS_CODEPAGE_949
#undef  CONFIG_NLS_CODEPAGE_874
#undef  CONFIG_NLS_ISO8859_8
#undef  CONFIG_NLS_CODEPAGE_1250
#undef  CONFIG_NLS_CODEPAGE_1251
#define CONFIG_NLS_ISO8859_1 1
#undef  CONFIG_NLS_ISO8859_2
#undef  CONFIG_NLS_ISO8859_3
#undef  CONFIG_NLS_ISO8859_4
#undef  CONFIG_NLS_ISO8859_5
#undef  CONFIG_NLS_ISO8859_6
#undef  CONFIG_NLS_ISO8859_7
#undef  CONFIG_NLS_ISO8859_9
#undef  CONFIG_NLS_ISO8859_13
#undef  CONFIG_NLS_ISO8859_14
#define CONFIG_NLS_ISO8859_15 1
#undef  CONFIG_NLS_KOI8_R
#undef  CONFIG_NLS_KOI8_U
#undef  CONFIG_NLS_UTF8

/*
 * Sound
 */
#undef  CONFIG_SOUND

/*
 * MPC8xx Options
 */

/*
 * Generic MPC8xx Options
 */
#define CONFIG_8xx_COPYBACK 1
#undef  CONFIG_8xx_CPU6
#undef  CONFIG_UCODE_PATCH

/*
 * MPC8xx CPM Options
 */
#undef  CONFIG_CPM_TRACK_LOAD
#undef  CONFIG_SCC_ENET
#undef  CONFIG_SCC1_ENET
#undef  CONFIG_SCC2_ENET
#undef  CONFIG_SCC3_ENET
#define CONFIG_FEC_ENET 1
#define CONFIG_USE_MDIO 1
#undef  CONFIG_FEC_GENERIC_PHY
#undef  CONFIG_FEC_AM79C874
#undef  CONFIG_FEC_LXT970
#define CONFIG_FEC_LXT971 1
#undef  CONFIG_FEC_QS6612
#undef  CONFIG_FEC_DP83843
#undef  CONFIG_FEC_DP83846A
#define CONFIG_ENET_BIG_BUFFERS 1
#undef  CONFIG_SMC1_UART
#undef  CONFIG_SMC2_UART
#define CONFIG_USE_SCC_IO 1
#define CONFIG_SCC1_UART 1
#define CONFIG_PORT_CTS1_NONE 1
#undef  CONFIG_UART_CTS_CONTROL_SCC1
#define CONFIG_PORT_RTS1_NONE 1
#undef  CONFIG_PORT_RTS1_B
#undef  CONFIG_PORT_RTS1_C
#define CONFIG_PORT_CD1_NONE 1
#undef  CONFIG_UART_CD_CONTROL_SCC1
#define CONFIG_PORT_DTR1_NONE 1
#undef  CONFIG_PORT_DTR1_A
#undef  CONFIG_PORT_DTR1_B
#undef  CONFIG_PORT_DTR1_C
#undef  CONFIG_PORT_DTR1_D
#define CONFIG_UART_MAXIDL_SCC1 (1)
#define CONFIG_SCC1_UART_RX_BDNUM (4)
#define CONFIG_SCC1_UART_RX_BDSIZE (32)
#define CONFIG_SCC1_UART_TX_BDNUM (4)
#define CONFIG_SCC1_UART_TX_BDSIZE (32)
#undef  CONFIG_SCC2_UART
#undef  CONFIG_SCC3_UART
#undef  CONFIG_SCC4_UART

/*
 * USB support
 */
#define CONFIG_USB 1
#undef  CONFIG_USB_DEBUG

/*
 * Miscellaneous USB options
 */
#define CONFIG_USB_DEVICEFS 1
#undef  CONFIG_USB_BANDWIDTH

/*
 * USB Host Controller Drivers
 */
#undef  CONFIG_USB_EHCI_HCD
#undef  CONFIG_USB_UHCI
#undef  CONFIG_USB_UHCI_ALT
#undef  CONFIG_USB_OHCI
#define CONFIG_USB_ISP1362 1

/*
 * USB Device Class drivers
 */
#undef  CONFIG_USB_AUDIO
#undef  CONFIG_USB_EMI26
#undef  CONFIG_USB_BLUETOOTH
#undef  CONFIG_USB_MIDI

/*
 *   SCSI support is needed for USB Storage
 */
#undef  CONFIG_USB_STORAGE
#undef  CONFIG_USB_STORAGE_DEBUG
#undef  CONFIG_USB_STORAGE_DATAFAB
#undef  CONFIG_USB_STORAGE_FREECOM
#undef  CONFIG_USB_STORAGE_ISD200
#undef  CONFIG_USB_STORAGE_DPCM
#undef  CONFIG_USB_STORAGE_HP8200e
#undef  CONFIG_USB_STORAGE_SDDR09
#undef  CONFIG_USB_STORAGE_SDDR55
#undef  CONFIG_USB_STORAGE_JUMPSHOT
#define CONFIG_USB_ACM 1
#undef  CONFIG_USB_PRINTER

/*
 * USB Human Interface Devices (HID)
 */
#undef  CONFIG_USB_HID

/*
 *     Input core support is needed for USB HID input layer or HIDBP support
 */
#undef  CONFIG_USB_HIDINPUT
#undef  CONFIG_USB_HIDDEV
#undef  CONFIG_USB_KBD
#undef  CONFIG_USB_MOUSE
#undef  CONFIG_USB_AIPTEK
#undef  CONFIG_USB_WACOM
#undef  CONFIG_USB_KBTAB
#undef  CONFIG_USB_POWERMATE

/*
 * USB Imaging devices
 */
#undef  CONFIG_USB_DC2XX
#undef  CONFIG_USB_MDC800
#undef  CONFIG_USB_SCANNER
#undef  CONFIG_USB_MICROTEK
#undef  CONFIG_USB_HPUSBSCSI

/*
 * USB Multimedia devices
 */

/*
 *   Video4Linux support is needed for USB Multimedia device support
 */

/*
 * USB Network adaptors
 */
#undef  CONFIG_USB_PEGASUS
#undef  CONFIG_USB_RTL8150
#undef  CONFIG_USB_KAWETH
#undef  CONFIG_USB_CATC
#undef  CONFIG_USB_AX8817X
#undef  CONFIG_USB_CDCETHER
#undef  CONFIG_USB_USBNET

/*
 * USB port drivers
 */
#undef  CONFIG_USB_USS720

/*
 * USB Serial Converter support
 */
#undef  CONFIG_USB_SERIAL

/*
 * USB Miscellaneous drivers
 */
#undef  CONFIG_USB_RIO500
#undef  CONFIG_USB_AUERSWALD
#undef  CONFIG_USB_TIGL
#undef  CONFIG_USB_BRLVGER
#undef  CONFIG_USB_LCD
#undef  CONFIG_ISP1362_USB
#undef  CONFIG_ISP1362_NETLINK

/*
 * Support for USB gadgets
 */
#undef  CONFIG_USB_GADGET

/*
 * Bluetooth support
 */
#undef  CONFIG_BLUEZ

/*
 * Cryptographic options
 */
#undef  CONFIG_CRYPTO

/*
 * Library routines
 */
#undef  CONFIG_CRC32
#define CONFIG_ZLIB_INFLATE 1
#define CONFIG_ZLIB_DEFLATE 1
#undef  CONFIG_FW_LOADER

/*
 * Kernel hacking
 */
#undef  CONFIG_DEBUG_KERNEL
#define CONFIG_LOG_BUF_SHIFT (0)
