/*
 * MPC850 microcode patch for relocating I2C/SPI parameters.
 * The patch allows concurrent operation of SCC1 Ethernet and I2C
 * and/or SCC2 Ethernet and SPI by solving the parameter RAM (PRAM) 
 * conflict caused by the fact that some of the Ethernet parameters 
 * overlay the I2C/SPI parameters.
 * The patch is applicable to the MPC850 processor only.
 * The patch (dated 06/10/2002, v1.3) is provided by Motorola at
 * MPC850 Product Summary Page.
 *
 * See the SMC/I2C/SPI patch later in this file.
 */
#ifdef USE_IIC_PATCH
#define PATCH_DEFINED
/* IIC/SPI */
uint patch_2000[] = {
	0x7fffefd9,
	0x3ffd0000,
	0x7ffb49f7,
	0x7ff90000,
	0x5fefadf7,
	0x5f88adf7,
	0x5fefaff7,
	0x5f88aff7,
	0x3a9cfbc8,
	0x77cae1bb,
	0xf4de7fad,
	0xabae9330,
	0x4e08fdcf,
	0x6e0faff8,
	0x7ccf76cf,
	0xfdaff9cf,
	0xabf88dc8,
	0xab5879f7,
	0xb0926a27,
	0xdfd079f7,
	0xb090e6bb,
	0xe5bbe74f,
	0xaa616f0f,
	0x6ffb76ce,
	0xee0cf9cf,
	0x2bfbefef,
	0xcfeef9cf,
	0x76cead23,
	0x90b3df99,
	0x7fddd0c1,
	0x4bf847fd,
	0x7ccf76ce,
	0xcfef77ca,
	0x7eaf7fad,
	0x7dfdf0b7,
	0xef7a7fca,
	0x77cafbc8,
	0x6079e722,
	0xfbc85fff,
	0xdfff5fb3,
	0xfffbfbc8,
	0xf3c894a5,
	0xe7c9edf9,
	0x7f9a7fad,
	0x5f36afe8,
	0x5f5bffdf,
	0xdf95cb9e,
	0xaf7d5fc3,
	0xafed8c1b,
	0x5fc3afdd,
	0x5fc5df99,
	0x7efdb0b3,
	0x5fb3fffe,
	0xabae5fb3,
	0xfffe5fd0,
	0x600be6bb,
	0x600b5fd0,
	0xdfc827fb,
	0xefdf5fca,
	0xcfde3a9c,
	0xe7c9edf9,
	0xf3c87f9e,
	0x54ca7fed,
	0x2d3a3637,
	0x756f7e9a,
	0xf1ce37ef,
	0x2e677fee,
	0x10ebadf8,
	0xefdecfea,
	0xe52f7d9f,
	0xe12bf1ce,
	0x5f647e9a,
	0x4df8cfea,
	0x5f717d9b,
	0xefeecfea,
	0x5f73e522,
	0xefde5f73,
	0xcfda0b61,
	0x6a29df61,
	0xe7c9edf9,
	0x7e9a30d5,
	0x1458bfff,
	0xf3c85fff,
	0xdfffa7f8,
	0x5f5bbffe,
	0x7f7d10d0,
	0x144d5f33,
	0xbfffaf78,
	0x5f5bbffd,
	0xa7f85f33,
	0xbffe77fd,
	0x30bd4e08,
	0xfdcfe5ff,
	0x6e0faff8,
	0x7eef7e9f,
	0xfdeff1cf,
	0x5f17abf8,
	0x0d5b5f5b,
	0xffef79f7,
	0x309eafdd,
	0x5f3147f8,
	0x5f31afed,
	0x7fdd50af,
	0x497847fd,
	0x7f9e7fed,
	0x7dfd70a9,
	0xef7e7ece,
	0x6ba07f9e,
	0x2d227efd,
	0x30db5f5b,
	0xfffd5f5b,
	0xffef5f5b,
	0xffdf0c9c,
	0xafed0a9a,
	0xafdd0c37,
	0x5f37afbd,
	0x7fbdb081,
	0x5f8147f8
};

uint patch_2f00[] = {
	0x3e303430,
	0x34343737,
	0xabbf9b99,
	0x4b4fbdbd,
	0x59949334,
	0x9fff37fb,
	0x9b177dd9,
	0x936956bb,
	0xfbdd697b,
	0xdd2fd113,
	0x1db9f7bb,
	0x36313963,
	0x79373369,
	0x3193137f,
	0x7331737a,
	0xf7bb9b99,
	0x9bb19795,
	0x77fdfd3d,
	0x573b773f,
	0x737933f7,
	0xb991d115,
	0x31699315,
	0x31531694,
	0xbf4fbdbd,
	0x35931497,
	0x35376956,
	0xbd697b9d,
	0x96931313,
	0x19797937,
	0x69350000
};
#endif


/*
 * MPC860 microcode patch for relocating SMC/I2C/SPI parameters.
 * The patch allows concurrent operation of Ethernet and SMC(UART)
 * and I2C/SPI by solving the parameter RAM conflict caused by the fact 
 * that some of the Ethernet parameters overlay the SMC(UART) and 
 * I2C/SPI parameters.
 * The patch applicable to the 850 and 823E processors.
 * The patch (dated 01/22/2002, v1.1) is provided by Motorola at
 * MPC850 Product Summary Page.
 */
#ifdef USE_SMC_PATCH
#define PATCH_DEFINED
/* SMC2/IIC/SPI Patch */
/* This is the area from 0x2000 to 0x23ff.
 */
uint patch_2000[] = {
	0x3fff0000,
	0x3ffd0000,
	0x3ffb0000,
	0x3ff90000,
	0x5f13eff8,
	0x5eb5eff8,
	0x5f88adf7,
	0x5fefadf7,
	0x3a9cfbc8,
	0x77cae1bb,
	0xf4de7fad,
	0xabae9330,
	0x4e08fdcf,
	0x6e0faff8,
	0x7ccf76cf,
	0xfdaff9cf,
	0xabf88dc8,
	0xab5879f7,
	0xb0925d8d,
	0xdfd079f7,
	0xb090e6bb,
	0xe5bbe74f,
	0x9e046f0f,
	0x6ffb76ce,
	0xee0cf9cf,
	0x2bfbefef,
	0xcfeef9cf,
	0x76cead23,
	0x90b3df99,
	0x7fddd0c1,
	0x4bf847fd,
	0x7ccf76ce,
	0xcfef77ca,
	0x7eaf7fad,
	0x7dfdf0b7,
	0xef7a7fca,
	0x77cafbc8,
	0x6079e722,
	0xfbc85fff,
	0xdfff5fb3,
	0xfffbfbc8,
	0xf3c894a5,
	0xe7c9edf9,
	0x7f9a7fad,
	0x5f36afe8,
	0x5f5bffdf,
	0xdf95cb9e,
	0xaf7d5fc3,
	0xafed8c1b,
	0x5fc3afdd,
	0x5fc5df99,
	0x7efdb0b3,
	0x5fb3fffe,
	0xabae5fb3,
	0xfffe5fd0,
	0x600be6bb,
	0x600b5fd0,
	0xdfc827fb,
	0xefdf5fca,
	0xcfde3a9c,
	0xe7c9edf9,
	0xf3c87f9e,
	0x54ca7fed,
	0x2d3a3637,
	0x756f7e9a,
	0xf1ce37ef,
	0x2e677fee,
	0x10ebadf8,
	0xefdecfea,
	0xe52f7d9f,
	0xe12bf1ce,
	0x5f647e9a,
	0x4df8cfea,
	0x5f717d9b,
	0xefeecfea,
	0x5f73e522,
	0xefde5f73,
	0xcfda0b61,
	0x5d8fdf61,
	0xe7c9edf9,
	0x7e9a30d5,
	0x1458bfff,
	0xf3c85fff,
	0xdfffa7f8,
	0x5f5bbffe,
	0x7f7d10d0,
	0x144d5f33,
	0xbfffaf78,
	0x5f5bbffd,
	0xa7f85f33,
	0xbffe77fd,
	0x30bd4e08,
	0xfdcfe5ff,
	0x6e0faff8,
	0x7eef7e9f,
	0xfdeff1cf,
	0x5f17abf8,
	0x0d5b5f5b,
	0xffef79f7,
	0x309eafdd,
	0x5f3147f8,
	0x5f31afed,
	0x7fdd50af,
	0x497847fd,
	0x7f9e7fed,
	0x7dfd70a9,
	0xef7e7ece,
	0x6ba07f9e,
	0x2d227efd,
	0x30db5f5b,
	0xfffd5f5b,
	0xffef5f5b,
	0xffdf0c9c,
	0xafed0a9a,
	0xafdd0c37,
	0x5f37afbd,
	0x7fbdb081,
	0x5f8147f8,
	0x3a11e710,
	0xedf0ccdd,
	0xf3186d0a,
	0x7f0e5f06,
	0x7fedbb38,
	0x3afe7468,
	0x7fedf4fc,
	0x8ffbb951,
	0xb85f77fd,
	0xb0df5ddd,
	0xdefe7fed,
	0x90e1e74d,
	0x6f0dcbf7,
	0xe7decfed,
	0xcb74cfed,
	0xcfeddf6d,
	0x91714f74,
	0x5dd2deef,
	0x9e04e7df,
	0xefbb6ffb,
	0xe7ef7f0e,
	0x9e097fed,
	0xebdbeffa,
	0xeb54affb,
	0x7fea90d7,
	0x7e0cf0c3,
	0xbffff318,
	0x5fffdfff,
	0xac59efea,
	0x7fce1ee5,
	0xe2ff5ee1,
	0xaffbe2ff,
	0x5ee3affb,
	0xf9cc7d0f,
	0xaef8770f,
	0x7d0fb0c6,
	0xeffbbfff,
	0xcfef5ede,
	0x7d0fbfff,
	0x5ede4cf8,
	0x7fddd0bf,
	0x49f847fd,
	0x7efdf0bb,
	0x7fedfffd,
	0x7dfdf0b7,
	0xef7e7e1e,
	0x5ede7f0e,
	0x3a11e710,
	0xedf0ccab,
	0xfb18ad2e,
	0x1ea9bbb8,
	0x74283b7e,
	0x73c2e4bb,
	0x2ada4fb8,
	0xdc21e4bb,
	0xb2a1ffbf,
	0x5e2c43f8,
	0xfc87e1bb,
	0xe74ffd91,
	0x6f0f4fe8,
	0xc7ba32e2,
	0xf396efeb,
	0x600b4f78,
	0xe5bb760b,
	0x53acaef8,
	0x4ef88b0e,
	0xcfef9e09,
	0xabf8751f,
	0xefef5bac,
	0x741f4fe8,
	0x751e760d,
	0x7fdbf081,
	0x741cafce,
	0xefcc7fce,
	0x751e70ac,
	0x741ce7bb,
	0x3372cfed,
	0xafdbefeb,
	0xe5bb760b,
	0x53f2aef8,
	0xafe8e7eb,
	0x4bf8771e,
	0x7e247fed,
	0x4fcbe2cc,
	0x7fbc30a9,
	0x7b0f7a0f,
	0x34d577fd,
	0x308b5db7,
	0xde553e5f,
	0xaf78741f,
	0x741f30f0,
	0xcfef5e2c,
	0x741f3eac,
	0xafb8771e,
	0x5e677fed,
	0x0bd3e2cc,
	0x741ccfec,
	0xe5ca53cd,
	0x6fcb4f74,
	0x5dadde4b,
	0x2ab63d38,
	0x4bb3de30,
	0x751f741c,
	0x6c42effa,
	0xefea7fce,
	0x6ffc30be,
	0xefec3fca,
	0x30b3de2e,
	0xadf85d9e,
	0xaf7daefd,
	0x5d9ede2e,
	0x5d9eafdd,
	0x761f10ac,
	0x1da07efd,
	0x30adfffe,
	0x4908fb18,
	0x5fffdfff,
	0xafbb709b,
	0x4ef85e67,
	0xadf814ad,
	0x7a0f70ad,
	0xcfef50ad,
	0x7a0fde30,
	0x5da0afed,
	0x3c12780f,
	0xefef780f,
	0xefef790f,
	0xa7f85e0f,
	0xffef790f,
	0xefef790f,
	0x14adde2e,
	0x5d9eadfd,
	0x5e2dfffb,
	0xe79addfd,
	0xeff96079,
	0x607ae79a,
	0xddfceff9,
	0x60795dff,
	0x607acfef,
	0xefefefdf,
	0xefbfef7f,
	0xeeffedff,
	0xebffe7ff,
	0xafefafdf,
	0xafbfaf7f,
	0xaeffadff,
	0xabffa7ff,
	0x6fef6fdf,
	0x6fbf6f7f,
	0x6eff6dff,
	0x6bff67ff,
	0x2fef2fdf,
	0x2fbf2f7f,
	0x2eff2dff,
	0x2bff27ff,
	0x4e08fd1f,
	0xe5ff6e0f,
	0xaff87eef,
	0x7e0ffdef,
	0xf11f6079,
	0xabf8f542,
	0x7e0af11c,
	0x37cfae3a,
	0x7fec90be,
	0xadf8efdc,
	0xcfeae52f,
	0x7d0fe12b,
	0xf11c6079,
	0x7e0a4df8,
	0xcfea5dc4,
	0x7d0befec,
	0xcfea5dc6,
	0xe522efdc,
	0x5dc6cfda,
	0x4e08fd1f,
	0x6e0faff8,
	0x7c1f761f,
	0xfdeff91f,
	0x6079abf8,
	0x761cee24,
	0xf91f2bfb,
	0xefefcfec,
	0xf91f6079,
	0x761c27fb,
	0xefdf5da7,
	0xcfdc7fdd,
	0xd09c4bf8,
	0x47fd7c1f,
	0x761ccfcf,
	0x7eef7fed,
	0x7dfdf093,
	0xef7e7f1e,
	0x771efb18,
	0x6079e722,
	0xe6bbe5bb,
	0xae0ae5bb,
	0x600bae85,
	0xe2bbe2bb,
	0xe2bbe2bb,
	0xaf02e2bb,
	0xe2bb2ff9,
	0x6079e2bb
};

/* This is from 0x2f00 to 0x2fff
 */
uint patch_2f00[] = {
	0x30303030,
	0x3e3e3434,
	0xabbf9b99,
	0x4b4fbdbd,
	0x59949334,
	0x9fff37fb,
	0x9b177dd9,
	0x936956bb,
	0xfbdd697b,
	0xdd2fd113,
	0x1db9f7bb,
	0x36313963,
	0x79373369,
	0x3193137f,
	0x7331737a,
	0xf7bb9b99,
	0x9bb19795,
	0x77fdfd3d,
	0x573b773f,
	0x737933f7,
	0xb991d115,
	0x31699315,
	0x31531694,
	0xbf4fbdbd,
	0x35931497,
	0x35376956,
	0xbd697b9d,
	0x96931313,
	0x19797937,
	0x6935af78,
	0xb9b3baa3,
	0xb8788683,
	0x368f78f7,
	0x87778733,
	0x3ffffb3b,
	0x8e8f78b8,
	0x1d118e13,
	0xf3ff3f8b,
	0x6bd8e173,
	0xd1366856,
	0x68d1687b,
	0x3daf78b8,
	0x3a3a3f87,
	0x8f81378f,
	0xf876f887,
	0x77fd8778,
	0x737de8d6,
	0xbbf8bfff,
	0xd8df87f7,
	0xfd876f7b,
	0x8bfff8bd,
	0x8683387d,
	0xb873d87b,
	0x3b8fd7f8,
	0xf7338883,
	0xbb8ee1f8,
	0xef837377,
	0x3337b836,
	0x817d11f8,
	0x7378b878,
	0xd3368b7d,
	0xed731b7d,
	0x833731f3,
	0xf22f3f23
};

uint patch_2e00[] = {
	/* This is from 0x2e00 to 0x2e3c
	 */
	0x27eeeeee,
	0xeeeeeeee,
	0xeeeeeeee,
	0xeeeeeeee,
	0xee4bf4fb,
	0xdbd259bb,
	0x1979577f,
	0xdfd2d573,
	0xb773f737,
	0x4b4fbdbd,
	0x25b9b177,
	0xd2d17376,
	0x956bbfdd,
	0x697bdd2f,
	0xff9f79ff,
	0xff9ff22f
};
#endif