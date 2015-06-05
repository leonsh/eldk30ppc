/*
 * This microcode patch provides automatic USB start-of-frame (SOF)
 * token generation functionality. Patch (dated 04/10/00) is provided 
 * by Motorola at MPC823 Product Summary Page.
 */

#ifdef USE_USB_SOF_PATCH
#define PATCH_DEFINED
uint patch_2000[] = {
	0x7fff0000,
	0x7ffd0000,
	0x7ffb0000,
	0x49f7ba5b,
	0xba383ffb,
	0xf9b8b46d,
	0xe5ab4e07,
	0xaf77bffe,
	0x3f7bbf79,
	0xba5bba38,
	0xe7676076,
	0x60750000
};

uint patch_2f00[] = {
	0x3030304c,
	0xcab9e441,
	0xa1aaf220
};
#endif
