/*
 * MPC860 microcode patch for relocating I2C/SPI parameters.
 * The patch allows concurrent operation of SCC1 Ethernet and I2C
 * and/or SCC2 Ethernet and SPI by solving the parameter RAM (PRAM) 
 * conflict caused by the fact that some of the Ethernet parameters 
 * overlay the I2C/SPI parameters.
 * The patch is applicable to all MPC8xx processors except MPC850.
 * The patch (dated 03/06/2002, v1.3) is provided by Motorola at
 * MPC860 Product Summary Page.
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
	0xb0927383,
	0xdfd079f7,
	0xb090e6bb,
	0xe5bbe74f,
	0xb3fa6f0f,
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
	0x7385df61,
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
 * The patch applicable to all 8xx family except 850 and 823E.
 * The patch (dated 01/22/2002, v1.1) is provided by Motorola at
 * MPC860 Product Summary Page.
 */
#ifdef USE_SMC_PATCH
#define PATCH_DEFINED
/* SMC2/IIC/SPI Patch */
/* This is the area from 0x2000 to 0x23ff.
 */
uint patch_2000[] = {
	0x3FFF0000,
	0x3FFD0000,
	0x3FFB0000,
	0x3FF90000,
	0x5F13EFF8,
	0x5EB5EFF8,
	0x5F88ADF7,
	0x5FEFADF7,
	0x3A9CFBC8,
	0x77CAE1BB,
	0xF4DE7FAD,
	0xABAE9330,
	0x4E08FDCF,
	0x6E0FAFF8,
	0x7CCF76CF,
	0xFDAFF9CF,
	0xABF88DC8,
	0xAB5879F7,
	0xB0925D8D,
	0xDFD079F7,
	0xB090E6BB,
	0xE5BBE74F,
	0x9E046F0F,
	0x6FFB76CE,
	0xEE0CF9CF,
	0x2BFBEFEF,
	0xCFEEF9CF,
	0x76CEAD23,
	0x90B3DF99,
	0x7FDDD0C1,
	0x4BF847FD,
	0x7CCF76CE,
	0xCFEF77CA,
	0x7EAF7FAD,
	0x7DFDF0B7,
	0xEF7A7FCA,
	0x77CAFBC8,
	0x6079E722,
	0xFBC85FFF,
	0xDFFF5FB3,
	0xFFFBFBC8,
	0xF3C894A5,
	0xE7C9EDF9,
	0x7F9A7FAD,
	0x5F36AFE8,
	0x5F5BFFDF,
	0xDF95CB9E,
	0xAF7D5FC3,
	0xAFED8C1B,
	0x5FC3AFDD,
	0x5FC5DF99,
	0x7EFDB0B3,
	0x5FB3FFFE,
	0xABAE5FB3,
	0xFFFE5FD0,
	0x600BE6BB,
	0x600B5FD0,
	0xDFC827FB,
	0xEFDF5FCA,
	0xCFDE3A9C,
	0xE7C9EDF9,
	0xF3C87F9E,
	0x54CA7FED,
	0x2D3A3637,
	0x756F7E9A,
	0xF1CE37EF,
	0x2E677FEE,
	0x10EBADF8,
	0xEFDECFEA,
	0xE52F7D9F,
	0xE12BF1CE,
	0x5F647E9A,
	0x4DF8CFEA,
	0x5F717D9B,
	0xEFEECFEA,
	0x5F73E522,
	0xEFDE5F73,
	0xCFDA0B61,
	0x5D8FDF61,
	0xE7C9EDF9,
	0x7E9A30D5,
	0x1458BFFF,
	0xF3C85FFF,
	0xDFFFA7F8,
	0x5F5BBFFE,
	0x7F7D10D0,
	0x144D5F33,
	0xBFFFAF78,
	0x5F5BBFFD,
	0xA7F85F33,
	0xBFFE77FD,
	0x30BD4E08,
	0xFDCFE5FF,
	0x6E0FAFF8,
	0x7EEF7E9F,
	0xFDEFF1CF,
	0x5F17ABF8,
	0x0D5B5F5B,
	0xFFEF79F7,
	0x309EAFDD,
	0x5F3147F8,
	0x5F31AFED,
	0x7FDD50AF,
	0x497847FD,
	0x7F9E7FED,
	0x7DFD70A9,
	0xEF7E7ECE,
	0x6BA07F9E,
	0x2D227EFD,
	0x30DB5F5B,
	0xFFFD5F5B,
	0xFFEF5F5B,
	0xFFDF0C9C,
	0xAFED0A9A,
	0xAFDD0C37,
	0x5F37AFBD,
	0x7FBDB081,
	0x5F8147F8,
	0x3A11E710,
	0xEDF0CCDD,
	0xF3186D0A,
	0x7F0E5F06,
	0x7FEDBB38,
	0x3AFE7468,
	0x7FEDF4FC,
	0x8FFBB951,
	0xB85F77FD,
	0xB0DF5DDD,
	0xDEFE7FED,
	0x90E1E74D,
	0x6F0DCBF7,
	0xE7DECFED,
	0xCB74CFED,
	0xCFEDDF6D,
	0x91714F74,
	0x5DD2DEEF,
	0x9E04E7DF,
	0xEFBB6FFB,
	0xE7EF7F0E,
	0x9E097FED,
	0xEBDBEFFA,
	0xEB54AFFB,
	0x7FEA90D7,
	0x7E0CF0C3,
	0xBFFFF318,
	0x5FFFDFFF,
	0xAC59EFEA,
	0x7FCE1EE5,
	0xE2FF5EE1,
	0xAFFBE2FF,
	0x5EE3AFFB,
	0xF9CC7D0F,
	0xAEF8770F,
	0x7D0FB0C6,
	0xEFFBBFFF,
	0xCFEF5EDE,
	0x7D0FBFFF,
	0x5EDE4CF8,
	0x7FDDD0BF,
	0x49F847FD,
	0x7EFDF0BB,
	0x7FEDFFFD,
	0x7DFDF0B7,
	0xEF7E7E1E,
	0x5EDE7F0E,
	0x3A11E710,
	0xEDF0CCAB,
	0xFB18AD2E,
	0x1EA9BBB8,
	0x74283B7E,
	0x7375E4BB,
	0x2ADA4FB8,
	0xDC21E4BB,
	0xB2A1FFBF,
	0x5E2C43F8,
	0xFC87E1BB,
	0xE74FFD91,
	0x6F0F4FE8,
	0xC7BA32E2,
	0xF396EFEB,
	0x600B4F78,
	0xE5BB760B,
	0x53ACAEF8,
	0x4EF88B0E,
	0xCFEF9E09,
	0xABF8751F,
	0xEFEF5BAC,
	0x741F4FE8,
	0x751E760D,
	0x7FDBF081,
	0x741CAFCE,
	0xEFCC7FCE,
	0x751E70AC,
	0x741CE7BB,
	0x3372CFED,
	0xAFDBEFEB,
	0xE5BB760B,
	0x53F2AEF8,
	0xAFE8E7EB,
	0x4BF8771E,
	0x7E247FED,
	0x4FCBE2CC,
	0x7FBC30A9,
	0x7B0F7A0F,
	0x34D577FD,
	0x308B5DB7,
	0xDE553E5F,
	0xAF78741F,
	0x741F30F0,
	0xCFEF5E2C,
	0x741F3EAC,
	0xAFB8771E,
	0x5E677FED,
	0x0BD3E2CC,
	0x741CCFEC,
	0xE5CA53CD,
	0x6FCB4F74,
	0x5DADDE4B,
	0x2AB63D38,
	0x4BB3DE30,
	0x751F741C,
	0x6C42EFFA,
	0xEFEA7FCE,
	0x6FFC30BE,
	0xEFEC3FCA,
	0x30B3DE2E,
	0xADF85D9E,
	0xAF7DAEFD,
	0x5D9EDE2E,
	0x5D9EAFDD,
	0x761F10AC,
	0x1DA07EFD,
	0x30ADFFFE,
	0x4908FB18,
	0x5FFFDFFF,
	0xAFBB709B,
	0x4EF85E67,
	0xADF814AD,
	0x7A0F70AD,
	0xCFEF50AD,
	0x7A0FDE30,
	0x5DA0AFED,
	0x3C12780F,
	0xEFEF780F,
	0xEFEF790F,
	0xA7F85E0F,
	0xFFEF790F,
	0xEFEF790F,
	0x14ADDE2E,
	0x5D9EADFD,
	0x5E2DFFFB,
	0xE79ADDFD,
	0xEFF96079,
	0x607AE79A,
	0xDDFCEFF9,
	0x60795DFF,
	0x607ACFEF,
	0xEFEFEFDF,
	0xEFBFEF7F,
	0xEEFFEDFF,
	0xEBFFE7FF,
	0xAFEFAFDF,
	0xAFBFAF7F,
	0xAEFFADFF,
	0xABFFA7FF,
	0x6FEF6FDF,
	0x6FBF6F7F,
	0x6EFF6DFF,
	0x6BFF67FF,
	0x2FEF2FDF,
	0x2FBF2F7F,
	0x2EFF2DFF,
	0x2BFF27FF,
	0x4E08FD1F,
	0xE5FF6E0F,
	0xAFF87EEF,
	0x7E0FFDEF,
	0xF11F6079,
	0xABF8F542,
	0x7E0AF11C,
	0x37CFAE3A,
	0x7FEC90BE,
	0xADF8EFDC,
	0xCFEAE52F,
	0x7D0FE12B,
	0xF11C6079,
	0x7E0A4DF8,
	0xCFEA5DC4,
	0x7D0BEFEC,
	0xCFEA5DC6,
	0xE522EFDC,
	0x5DC6CFDA,
	0x4E08FD1F,
	0x6E0FAFF8,
	0x7C1F761F,
	0xFDEFF91F,
	0x6079ABF8,
	0x761CEE24,
	0xF91F2BFB,
	0xEFEFCFEC,
	0xF91F6079,
	0x761C27FB,
	0xEFDF5DA7,
	0xCFDC7FDD,
	0xD09C4BF8,
	0x47FD7C1F,
	0x761CCFCF,
	0x7EEF7FED,
	0x7DFDF093,
	0xEF7E7F1E,
	0x771EFB18,
	0x6079E722,
	0xE6BBE5BB,
	0xAE0AE5BB,
	0x600BAE85,
	0xE2BBE2BB,
	0xE2BBE2BB,
	0xAF02E2BB,
	0xE2BB2FF9,
	0x6079E2BB
};

/* This is from 0x2f00 to 0x2fff
 */
uint patch_2f00[] = {
	0x30303030,
	0x3E3E3434,
	0xABBF9B99,
	0x4B4FBDBD,
	0x59949334,
	0x9FFF37FB,
	0x9B177DD9,
	0x936956BB,
	0xFBDD697B,
	0xDD2FD113,
	0x1DB9F7BB,
	0x36313963,
	0x79373369,
	0x3193137F,
	0x7331737A,
	0xF7BB9B99,
	0x9BB19795,
	0x77FDFD3D,
	0x573B773F,
	0x737933F7,
	0xB991D115,
	0x31699315,
	0x31531694,
	0xBF4FBDBD,
	0x35931497,
	0x35376956,
	0xBD697B9D,
	0x96931313,
	0x19797937,
	0x6935AF78,
	0xB9B3BAA3,
	0xB8788683,
	0x368F78F7,
	0x87778733,
	0x3FFFFB3B,
	0x8E8F78B8,
	0x1D118E13,
	0xF3FF3F8B,
	0x6BD8E173,
	0xD1366856,
	0x68D1687B,
	0x3DAF78B8,
	0x3A3A3F87,
	0x8F81378F,
	0xF876F887,
	0x77FD8778,
	0x737DE8D6,
	0xBBF8BFFF,
	0xD8DF87F7,
	0xFD876F7B,
	0x8BFFF8BD,
	0x8683387D,
	0xB873D87B,
	0x3B8FD7F8,
	0xF7338883,
	0xBB8EE1F8,
	0xEF837377,
	0x3337B836,
	0x817D11F8,
	0x7378B878,
	0xD3368B7D,
	0xED731B7D,
	0x833731F3,
	0xF22F3F23
};

uint patch_2e00[] = {
	/* This is from 0x2e00 to 0x2e3c
	 */
	0x27EEEEEE,
	0xEEEEEEEE,
	0xEEEEEEEE,
	0xEEEEEEEE,
	0xEE4BF4FB,
	0xDBD259BB,
	0x1979577F,
	0xDFD2D573,
	0xB773F737,
	0x4B4FBDBD,
	0x25B9B177,
	0xD2D17376,
	0x956BBFDD,
	0x697BDD2F,
	0xFF9F79FF,
	0xFF9FF22F
};
#endif