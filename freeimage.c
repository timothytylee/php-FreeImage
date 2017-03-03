/*
  +----------------------------------------------------------------------+
  | PHP Version 4                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2003 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 2.02 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available at through the world-wide-web at                           |
  | http://www.php.net/license/2_02.txt.                                 |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Wenlong Wu <ezdevelop@hotmail.com>                          |
  |          Noon       <noon@ms8.url.com.tw>                            |
  +----------------------------------------------------------------------+

  $Id: freeimage.c,v 1.7 2004/07/22 13:32:54 wenlong Exp $ 
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/php_standard.h"
#include "ext/standard/info.h"
#include "FreeImage.h"
#include "php_freeimage.h"
#include "php_freeimage_io.h"

ZEND_DECLARE_MODULE_GLOBALS(freeimage)

/* True global resources - no need for thread safety here */
static int le_freeimage_bitmap;
static int le_freeimage_multibitmap;
#define le_freeimage_name_bitmap		"FreeImage Bitmap Handle"
#define le_freeimage_name_multibitmap	"FreeImage Multipage Bitmap Handle"

zend_class_entry freeimage_class_entry;
static zend_class_entry *freeimage_class_entry_ptr;

/* {{{ _php_freeimage_error_handler
 */
void _php_freeimage_error_handler(FREE_IMAGE_FORMAT fif, const char *message)
{
	TSRMLS_FETCH();
	char *error_msg;
	int error_len = 1;
	error_msg = estrdup("");

	if (FREEIMAGE_G(freeimage_lasterror) != NULL) {
		efree(FREEIMAGE_G(freeimage_lasterror));
		FREEIMAGE_G(freeimage_lasterror) = NULL;
	}
	
	error_len = strlen(FreeImage_GetFormatFromFIF(fif)) + strlen(" Format: ") + strlen(message) + 3;
	error_msg = (char *)erealloc(error_msg, error_len);
	if (!error_msg) {
		php_printf("Couldn't allocate memery!");
		return;
	}
	strcat(error_msg, FreeImage_GetFormatFromFIF(fif));
	strcat(error_msg, " Format: ");
	strcat(error_msg, message);
	//sprintf(error_msg, "%s Format: %s", FreeImage_GetFormatFromFIF(fif), message);
	FREEIMAGE_G(freeimage_lasterror) = estrndup(error_msg, error_len);
	efree(error_msg);
	//php_printf("\n***\n%s\n***\n", FREEIMAGE_G(freeimage_lasterror));
}
/* }}} */

/* {{{ _php_freeimage_setio
 */
void _php_freeimage_setio(FreeImageIO *io, unsigned from) {
	switch (from) {
		case FREEIMAGEIO_FROM_MEMORY:
			io->read_proc  = FreeImageIO_MemoryReadProc;
			io->seek_proc  = FreeImageIO_MemorySeekProc;
			io->tell_proc  = FreeImageIO_MemoryTellProc;
			//io->write_proc = FreeImageIO_MemoryWriteProc;
			io->write_proc = NULL; // not needed for loading
			break;
		case FREEIMAGEIO_FROM_FILE:
		default:
			io->read_proc  = FreeImageIO_FileReadProc;
			io->seek_proc  = FreeImageIO_FileSeekProc;
			io->tell_proc  = FreeImageIO_FileTellProc;
			//io->write_proc = FreeImageIO_FileWriteProc;
			io->write_proc = NULL; // not needed for loading
			break;
	}
}
/* }}} */

/* {{{ _php_freeimage_color_to_rgbquad
 */
static void _php_freeimage_color_to_rgbquad(char *color)
{
	RGBQUAD *rgb = NULL;
	// not yet implemented
}
/* }}} */

/* {{{ php_freeimage_ctor & php_freeimage_dtor
 */
#ifdef PHP_WIN32
static void php_freeimage_ctor() {
	/* FreeImage error handler */
	FreeImage_SetOutputMessage(_php_freeimage_error_handler);
	return;
}

static void php_freeimage_dtor() {
	return;
}
#else
// call this ONLY when linking with FreeImage as a static library
static void php_freeimage_ctor() {
	FreeImage_Initialise(FALSE);
	/* FreeImage error handler */
	FreeImage_SetOutputMessage(_php_freeimage_error_handler);
}

static void php_freeimage_dtor() {
	FreeImage_DeInitialise();
}
#endif
/* }}} */

/* {{{ freeimage_functions[]
 *
 * Every user visible function must have an entry in freeimage_functions[].
 */
function_entry freeimage_functions[] = {
	/* Bitmap management functions */
	PHP_FE(freeimage_load,					NULL)
	PHP_FE(freeimage_save,					NULL)
	PHP_FALIAS(freeimage_free,	freeimage_unload,	NULL)
	PHP_FE(freeimage_clone,					NULL)
	/* General functions */
	PHP_FE(freeimage_getversion,			NULL)
	PHP_FE(freeimage_getcopyrightmessage,	NULL)
	PHP_FE(freeimage_getlasterror,			NULL)
	/* Bitmap infomation functions */
	PHP_FE(freeimage_getimagetype,			NULL)
	PHP_FE(freeimage_getcolorsused,			NULL)
	PHP_FE(freeimage_getbpp,				NULL)
	PHP_FE(freeimage_getwidth,				NULL)
	PHP_FE(freeimage_getheight,				NULL)
	PHP_FE(freeimage_getline,				NULL)
	PHP_FE(freeimage_getpitch,				NULL)
	PHP_FE(freeimage_getdibsize,			NULL)
	/* Filetype functions */
	PHP_FE(freeimage_getfiletype,			NULL)
	/* Plugin functions */
	PHP_FE(freeimage_getfifcount,			NULL)
	PHP_FE(freeimage_setpluginenabled,		NULL)
	PHP_FE(freeimage_ispluginenabled,		NULL)
	PHP_FE(freeimage_getfiffromformat,		NULL)
	PHP_FE(freeimage_getfiffrommime,		NULL)
	PHP_FE(freeimage_getformatfromfif,		NULL)
	PHP_FE(freeimage_getfifextensionlist,	NULL)
	PHP_FE(freeimage_getfifdescription,		NULL)
	PHP_FE(freeimage_getfifregexpr,			NULL)
	PHP_FE(freeimage_getfiffromfilename,	NULL)
	PHP_FE(freeimage_fifsupportsreading,	NULL)
	PHP_FE(freeimage_fifsupportswriting,	NULL)
	/* Tookit & Conversion functions */
	PHP_FE(freeimage_rotate,				NULL)
	PHP_FE(freeimage_rotateex,				NULL)
	PHP_FE(freeimage_fliphorizontal,		NULL)
	PHP_FE(freeimage_flipvertical,			NULL)
	PHP_FE(freeimage_rescale,				NULL)
	PHP_FE(freeimage_gamma,					NULL)
	PHP_FE(freeimage_brightness,			NULL)
	PHP_FE(freeimage_contrast,				NULL)
	PHP_FE(freeimage_invert,				NULL)
	PHP_FE(freeimage_copy,					NULL)
	PHP_FE(freeimage_paste,					NULL)
	PHP_FE(freeimage_composite,				NULL)
	PHP_FE(freeimage_convertto8bits,		NULL)
	PHP_FE(freeimage_convertto16bits555,	NULL)
	PHP_FE(freeimage_convertto16bits565,	NULL)
	PHP_FE(freeimage_convertto24bits,		NULL)
	PHP_FE(freeimage_convertto32bits,		NULL)
	PHP_FE(freeimage_colorquantize,			NULL)
	PHP_FE(freeimage_threshold,				NULL)
	PHP_FE(freeimage_dither,				NULL)
	/* Multipage function */
	PHP_FE(freeimage_openmultibitmap,		NULL)
	PHP_FE(freeimage_closemultibitmap,		NULL)
	PHP_FE(freeimage_getpagecount,			NULL)
	PHP_FE(freeimage_appendpage,			NULL)
	PHP_FE(freeimage_insertpage,			NULL)
	PHP_FE(freeimage_deletepage,			NULL)
	PHP_FE(freeimage_lockpage,				NULL)
	PHP_FE(freeimage_unlockpage,			NULL)
	PHP_FE(freeimage_movepage,				NULL)
	PHP_FE(freeimage_getlockedpagenumbers,	NULL)
	{NULL, NULL, NULL}	/* Must be the last line in freeimage_functions[] */
};
/* }}} */

/* {{{ freeimage_class_functions[]
 */
zend_function_entry freeimage_class_functions[] = {
	PHP_FALIAS(freeimage,			freeimage_constructor,			NULL)
	/* Bitmap management functions */
	PHP_FALIAS(load,				freeimage_load,					NULL)
	PHP_FALIAS(save,				freeimage_save,					NULL)
	PHP_FALIAS(free,				freeimage_unload,				NULL)
	PHP_FALIAS(clone,				freeimage_clone,				NULL)
	/* General functions */
	PHP_FALIAS(getversion,			freeimage_getversion,			NULL)
	PHP_FALIAS(getcopyrightmessage,	freeimage_getcopyrightmessage,	NULL)
	PHP_FALIAS(getlasterror,		freeimage_getlasterror,			NULL)
	/* Bitmap infomation functions */
	PHP_FALIAS(getimagetype,		freeimage_getimagetype,			NULL)
	PHP_FALIAS(getcolorsused,		freeimage_getcolorsused,		NULL)
	PHP_FALIAS(getbpp,				freeimage_getbpp,				NULL)
	PHP_FALIAS(getwidth,			freeimage_getwidth,				NULL)
	PHP_FALIAS(getheight,			freeimage_getheight,			NULL)
	PHP_FALIAS(getline,				freeimage_getline,				NULL)
	PHP_FALIAS(getpitch,			freeimage_getpitch,				NULL)
	PHP_FALIAS(getdibsize,			freeimage_getdibsize,			NULL)
	/* Filetype functions */
	PHP_FALIAS(getfiletype,			freeimage_getfiletype,			NULL)
	/* Plugin functions */
	PHP_FALIAS(getfifcount,			freeimage_getfifcount,			NULL)
	PHP_FALIAS(setpluginenabled,	freeimage_setpluginenabled,		NULL)
	PHP_FALIAS(ispluginenabled,		freeimage_ispluginenabled,		NULL)
	PHP_FALIAS(getfiffromformat,	freeimage_getfiffromformat,		NULL)
	PHP_FALIAS(getfiffrommime,		freeimage_getfiffrommime,		NULL)
	PHP_FALIAS(getformatfromfif,	freeimage_getformatfromfif,		NULL)
	PHP_FALIAS(getfifextensionlist,	freeimage_getfifextensionlist,	NULL)
	PHP_FALIAS(getfifdescription,	freeimage_getfifdescription,	NULL)
	PHP_FALIAS(getfifregexpr,		freeimage_getfifregexpr,		NULL)
	PHP_FALIAS(getfiffromfilename,	freeimage_getfiffromfilename,	NULL)
	PHP_FALIAS(fifsupportsreading,	freeimage_fifsupportsreading,	NULL)
	PHP_FALIAS(fifsupportswriting,	freeimage_fifsupportswriting,	NULL)
	/* Tookit & Conversion functions */
	PHP_FALIAS(rotate,				freeimage_rotate,				NULL)
	PHP_FALIAS(rotateex,			freeimage_rotateex,				NULL)
	PHP_FALIAS(fliphorizontal,		freeimage_fliphorizontal,		NULL)
	PHP_FALIAS(flipvertical,		freeimage_flipvertical,			NULL)
	PHP_FALIAS(rescale,				freeimage_rescale,				NULL)
	PHP_FALIAS(gamma,				freeimage_gamma,				NULL)
	PHP_FALIAS(brightness,			freeimage_brightness,			NULL)
	PHP_FALIAS(contrast,			freeimage_contrast,				NULL)
	PHP_FALIAS(invert,				freeimage_invert,				NULL)
	PHP_FALIAS(copy,				freeimage_copy,					NULL)
	PHP_FALIAS(parse,				freeimage_paste,				NULL)
	PHP_FALIAS(composite,			freeimage_composite,			NULL)
	PHP_FALIAS(convertto8bits,		freeimage_convertto8bits,		NULL)
	PHP_FALIAS(convertto16bits555,	freeimage_convertto16bits555,	NULL)
	PHP_FALIAS(convertto16bits565,	freeimage_convertto16bits565,	NULL)
	PHP_FALIAS(convertto24bits,		freeimage_convertto24bits,		NULL)
	PHP_FALIAS(convertto32bits,		freeimage_convertto32bits,		NULL)
	PHP_FALIAS(colorquantize,		freeimage_colorquantize,		NULL)
	PHP_FALIAS(threshold,			freeimage_threshold,			NULL)
	PHP_FALIAS(dither,				freeimage_dither,				NULL)
	/* Multipage function */
	PHP_FALIAS(openmultibitmap,		freeimage_openmultibitmap,		NULL)
	PHP_FALIAS(closemultibitmap,	freeimage_closemultibitmap,		NULL)
	PHP_FALIAS(getpagecount,		freeimage_getpagecount,			NULL)
	PHP_FALIAS(appendpage,			freeimage_appendpage,			NULL)
	PHP_FALIAS(insertpage,			freeimage_insertpage,			NULL)
	PHP_FALIAS(deletepage,			freeimage_deletepage,			NULL)
	PHP_FALIAS(lockpage,			freeimage_lockpage,				NULL)
	PHP_FALIAS(unlockpage,			freeimage_unlockpage,			NULL)
	PHP_FALIAS(movepage,			freeimage_movepage,				NULL)
	PHP_FALIAS(getlockedpagenumbers,freeimage_getlockedpagenumbers,	NULL)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ freeimage_module_entry
 */
zend_module_entry freeimage_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"freeimage",
	freeimage_functions,
	PHP_MINIT(freeimage),
	PHP_MSHUTDOWN(freeimage),
	PHP_RINIT(freeimage),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(freeimage),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(freeimage),
#if ZEND_MODULE_API_NO >= 20010901
	PECL_FREEIMAGE_VERSION, /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_FREEIMAGE
ZEND_GET_MODULE(freeimage)
#endif

/* {{{ _php_freeimage_bitmap_dtor
 */
static void _php_freeimage_bitmap_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	FIBITMAP *bitmap = (FIBITMAP *)rsrc->ptr;
	FreeImage_Unload(bitmap);
}
/* }}} */

/* {{{ _php_freeimage_multibitmap_dtor
 */
static void _php_freeimage_multibitmap_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	FIMULTIBITMAP *multibitmap = (FIMULTIBITMAP *)rsrc->ptr;
	FreeImage_CloseMultiBitmap(multibitmap, 0);
}
/* }}} */

/* {{{ php_freeimage_init_globals
 */
static void php_freeimage_init_globals(zend_freeimage_globals *freeimage_globals)
{
	freeimage_globals->freeimage_lasterror = NULL;
}
/* }}} */

#define REGISTER_FREEIMAGE_CONSTANT(__c) REGISTER_LONG_CONSTANT(#__c, __c, CONST_CS | CONST_PERSISTENT)

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(freeimage)
{
	ZEND_INIT_MODULE_GLOBALS(freeimage, php_freeimage_init_globals, NULL);
	
	le_freeimage_bitmap = zend_register_list_destructors_ex(_php_freeimage_bitmap_dtor, NULL, le_freeimage_name_bitmap, module_number);
	le_freeimage_multibitmap = zend_register_list_destructors_ex(_php_freeimage_multibitmap_dtor, NULL, le_freeimage_name_multibitmap, module_number);
	/* Register class */
	INIT_CLASS_ENTRY(freeimage_class_entry, "freeimage", freeimage_class_functions);	
	freeimage_class_entry_ptr = zend_register_internal_class(&freeimage_class_entry TSRMLS_CC);

	// Register constant. These values are taken from FreeImage.h in the FreeImage API
	/* ICC profile support */
	REGISTER_FREEIMAGE_CONSTANT(FIICC_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(FIICC_COLOR_IS_CMYK);

	/* I/O image format identifiers. */
	REGISTER_FREEIMAGE_CONSTANT(FIF_UNKNOWN);
	REGISTER_FREEIMAGE_CONSTANT(FIF_BMP);
	REGISTER_FREEIMAGE_CONSTANT(FIF_ICO);
	REGISTER_FREEIMAGE_CONSTANT(FIF_JPEG);
	REGISTER_FREEIMAGE_CONSTANT(FIF_JNG);
	REGISTER_FREEIMAGE_CONSTANT(FIF_KOALA);
	REGISTER_FREEIMAGE_CONSTANT(FIF_LBM);
	REGISTER_FREEIMAGE_CONSTANT(FIF_IFF);
	REGISTER_FREEIMAGE_CONSTANT(FIF_MNG);
	REGISTER_FREEIMAGE_CONSTANT(FIF_PBM);
	REGISTER_FREEIMAGE_CONSTANT(FIF_PBMRAW);
	REGISTER_FREEIMAGE_CONSTANT(FIF_PCD);
	REGISTER_FREEIMAGE_CONSTANT(FIF_PCX);
	REGISTER_FREEIMAGE_CONSTANT(FIF_PGM);
	REGISTER_FREEIMAGE_CONSTANT(FIF_PGMRAW);
	REGISTER_FREEIMAGE_CONSTANT(FIF_PNG);
	REGISTER_FREEIMAGE_CONSTANT(FIF_PPM);
	REGISTER_FREEIMAGE_CONSTANT(FIF_PPMRAW);
	REGISTER_FREEIMAGE_CONSTANT(FIF_RAS);
	REGISTER_FREEIMAGE_CONSTANT(FIF_TARGA);
	REGISTER_FREEIMAGE_CONSTANT(FIF_TIFF);
	REGISTER_FREEIMAGE_CONSTANT(FIF_WBMP);
	REGISTER_FREEIMAGE_CONSTANT(FIF_PSD);
	REGISTER_FREEIMAGE_CONSTANT(FIF_CUT);
	REGISTER_FREEIMAGE_CONSTANT(FIF_XBM);
	REGISTER_FREEIMAGE_CONSTANT(FIF_XPM);
	REGISTER_FREEIMAGE_CONSTANT(FIF_DDS);
	REGISTER_FREEIMAGE_CONSTANT(FIF_GIF);

	/* Image type used in FreeImage. */
	REGISTER_FREEIMAGE_CONSTANT(FIT_UNKNOWN);		// unknown type
	REGISTER_FREEIMAGE_CONSTANT(FIT_BITMAP);		// standard image			: 1-, 4-, 8-, 16-, 24-, 32-bit
	REGISTER_FREEIMAGE_CONSTANT(FIT_UINT16);		// array of unsigned short	: unsigned 16-bit
	REGISTER_FREEIMAGE_CONSTANT(FIT_INT16);			// array of short			: signed 16-bit
	REGISTER_FREEIMAGE_CONSTANT(FIT_UINT32);		// array of unsigned long	: unsigned 32-bit
	REGISTER_FREEIMAGE_CONSTANT(FIT_INT32);			// array of long			: signed 32-bit
	REGISTER_FREEIMAGE_CONSTANT(FIT_FLOAT);			// array of float			: 32-bit IEEE floating point
	REGISTER_FREEIMAGE_CONSTANT(FIT_DOUBLE);		// array of double			: 64-bit IEEE floating point
	REGISTER_FREEIMAGE_CONSTANT(FIT_COMPLEX);		// array of FICOMPLEX		: 2 x 64-bit IEEE floating point

	/* Image color type used in FreeImage. */
	REGISTER_FREEIMAGE_CONSTANT(FIC_MINISWHITE);	// min value is white
	REGISTER_FREEIMAGE_CONSTANT(FIC_MINISBLACK);	// min value is black
	REGISTER_FREEIMAGE_CONSTANT(FIC_RGB);			// RGB color model
	REGISTER_FREEIMAGE_CONSTANT(FIC_PALETTE);		// color map indexed
	REGISTER_FREEIMAGE_CONSTANT(FIC_RGBALPHA);		// RGB color model with alpha channel
	REGISTER_FREEIMAGE_CONSTANT(FIC_CMYK);			// CMYK color model

	/* Color quantization algorithms. Constants used in FreeImage_ColorQuantize. */
	REGISTER_FREEIMAGE_CONSTANT(FIQ_WUQUANT);		// Xiaolin Wu color quantization algorithm
	REGISTER_FREEIMAGE_CONSTANT(FIQ_NNQUANT);		// NeuQuant neural-net quantization algorithm by Anthony Dekker

	/* Dithering algorithms. Constants used FreeImage_Dither. */
	REGISTER_FREEIMAGE_CONSTANT(FID_FS);			// Floyd & Steinberg error diffusion
	REGISTER_FREEIMAGE_CONSTANT(FID_BAYER4x4);		// Bayer ordered dispersed dot dithering (order 2 dithering matrix)
	REGISTER_FREEIMAGE_CONSTANT(FID_BAYER8x8);		// Bayer ordered dispersed dot dithering (order 3 dithering matrix)
	REGISTER_FREEIMAGE_CONSTANT(FID_CLUSTER6x6);	// Ordered clustered dot dithering (order 3 - 6x6 matrix)
	REGISTER_FREEIMAGE_CONSTANT(FID_CLUSTER8x8);	// Ordered clustered dot dithering (order 4 - 8x8 matrix)
	REGISTER_FREEIMAGE_CONSTANT(FID_CLUSTER16x16);	// Ordered clustered dot dithering (order 8 - 16x16 matrix)

	/* Upsampling / downsampling filters. Constants used in FreeImage_Rescale. */
	REGISTER_FREEIMAGE_CONSTANT(FILTER_BOX);		// Box, pulse, Fourier window, 1st order (constant) b-spline
	REGISTER_FREEIMAGE_CONSTANT(FILTER_BICUBIC);	// Mitchell & Netravali's two-param cubic filter
	REGISTER_FREEIMAGE_CONSTANT(FILTER_BILINEAR);	// Bilinear filter
	REGISTER_FREEIMAGE_CONSTANT(FILTER_BSPLINE);	// 4th order (cubic) b-spline
	REGISTER_FREEIMAGE_CONSTANT(FILTER_CATMULLROM);	// Catmull-Rom spline, Overhauser spline
	REGISTER_FREEIMAGE_CONSTANT(FILTER_LANCZOS3);	// Lanczos3 filter

	/* Color channels. Constants used in color manipulation routines. */
	REGISTER_FREEIMAGE_CONSTANT(FICC_RGB);			// Use red, green and blue channels
	REGISTER_FREEIMAGE_CONSTANT(FICC_RED);			// Use red channel
	REGISTER_FREEIMAGE_CONSTANT(FICC_GREEN);		// Use green channel
	REGISTER_FREEIMAGE_CONSTANT(FICC_BLUE);			// Use blue channel
	REGISTER_FREEIMAGE_CONSTANT(FICC_ALPHA);		// Use alpha channel
	REGISTER_FREEIMAGE_CONSTANT(FICC_BLACK);		// Use black channel
	REGISTER_FREEIMAGE_CONSTANT(FICC_REAL);			// Complex images: use real part
	REGISTER_FREEIMAGE_CONSTANT(FICC_IMAG);			// Complex images: use imaginary part
	REGISTER_FREEIMAGE_CONSTANT(FICC_MAG);			// Complex images: use magnitude
	REGISTER_FREEIMAGE_CONSTANT(FICC_PHASE);		// Complex images: use phase

	/* Load / Save flag constants */
	REGISTER_FREEIMAGE_CONSTANT(BMP_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(BMP_SAVE_RLE);
	REGISTER_FREEIMAGE_CONSTANT(CUT_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(DDS_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(GIF_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(ICO_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(ICO_MAKEALPHA);		// convert to 32bpp and create an alpha channel from the AND-mask when loading
	REGISTER_FREEIMAGE_CONSTANT(IFF_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_FAST);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_ACCURATE);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_QUALITYSUPERB);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_QUALITYGOOD);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_QUALITYNORMAL);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_QUALITYAVERAGE);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_QUALITYBAD);
	REGISTER_FREEIMAGE_CONSTANT(KOALA_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(LBM_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(MNG_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(PCD_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(PCD_BASE);			// load the bitmap sized 768 x 512
	REGISTER_FREEIMAGE_CONSTANT(PCD_BASEDIV4);		// load the bitmap sized 384 x 256
	REGISTER_FREEIMAGE_CONSTANT(PCD_BASEDIV16);		// load the bitmap sized 192 x 128
	REGISTER_FREEIMAGE_CONSTANT(PCX_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(PNG_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(PNG_IGNOREGAMMA);	// avoid gamma correction
	REGISTER_FREEIMAGE_CONSTANT(PNM_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(PNM_SAVE_RAW);		// If set the writer saves in RAW format (i.e. P4, P5 or P6)
	REGISTER_FREEIMAGE_CONSTANT(PNM_SAVE_ASCII);	// If set the writer saves in ASCII format (i.e. P1, P2 or P3)
	REGISTER_FREEIMAGE_CONSTANT(PSD_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(RAS_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(TARGA_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(TARGA_LOAD_RGB888);	// If set the loader converts RGB555 and ARGB8888 -> RGB888.
	REGISTER_FREEIMAGE_CONSTANT(TIFF_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(TIFF_CMYK);			// reads/stores tags for separated CMYK (use | to combine with compression flags)
	REGISTER_FREEIMAGE_CONSTANT(TIFF_PACKBITS);		// save using PACKBITS compression
	REGISTER_FREEIMAGE_CONSTANT(TIFF_DEFLATE);		// save using DEFLATE compression (a.k.a. ZLIB compression)
	REGISTER_FREEIMAGE_CONSTANT(TIFF_ADOBE_DEFLATE);// save using ADOBE DEFLATE compression
	REGISTER_FREEIMAGE_CONSTANT(TIFF_NONE);			// save without any compression
	REGISTER_FREEIMAGE_CONSTANT(TIFF_CCITTFAX3);	// save using CCITT Group 3 fax encoding
	REGISTER_FREEIMAGE_CONSTANT(TIFF_CCITTFAX4);	// save using CCITT Group 4 fax encoding
	REGISTER_FREEIMAGE_CONSTANT(TIFF_LZW);			// save using LZW compression
	REGISTER_FREEIMAGE_CONSTANT(WBMP_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(XBM_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(XPM_DEFAULT);

	

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(freeimage)
{
	
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(freeimage)
{
	FREEIMAGE_G(freeimage_lasterror) = NULL;
	php_freeimage_ctor();
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(freeimage)
{
	if (FREEIMAGE_G(freeimage_lasterror) != NULL) {
		efree(FREEIMAGE_G(freeimage_lasterror));
	}
	php_freeimage_dtor();
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(freeimage)
{
	int i;
	char buf[64];

	php_info_print_table_start();
	php_info_print_table_header(2, "FreeImage support", "enabled");
	php_info_print_table_row(2, "FreeImage version", (char *)FreeImage_GetVersion());
	php_info_print_table_row(2, "PHP FreeImage version", PECL_FREEIMAGE_VERSION " $Id: freeimage.c,v 1.7 2004/07/22 13:32:54 wenlong Exp $");
	php_info_print_table_end();

	php_info_print_table_start();
	php_info_print_table_header(3, "Supported image formats", "Read Support", "Write Support");
	for (i=0; i<FreeImage_GetFIFCount(); i++)
	{
		sprintf(buf, "%s (%s)", FreeImage_GetFIFDescription((FREE_IMAGE_FORMAT)i), 
			FreeImage_GetFIFExtensionList((FREE_IMAGE_FORMAT)i));
		php_info_print_table_row(3, buf, FreeImage_FIFSupportsReading((FREE_IMAGE_FORMAT)i) ? "On" : "Off", 
				FreeImage_FIFSupportsWriting((FREE_IMAGE_FORMAT)i) ? "On" : "Off");
	}
	php_info_print_table_end();
}
/* }}} */

/* {{{ proto object freeimage_constructor()
   Constructor. */
PHP_FUNCTION(freeimage_constructor)
{
	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}
	add_property_string(this_ptr, "_version", PECL_FREEIMAGE_VERSION, 1);
	add_property_string(this_ptr, "_author", PECL_FREEIMAGE_AUTHOR, 1);
}
/* }}} */

/* {{{ proto resource freeimage_load(string filename [, int flags])
   Decodes a bitmap, allocates memory for it and then returns it as a FIBITMAP. */
PHP_FUNCTION(freeimage_load)
{
	char *filename = NULL;
	int argc = ZEND_NUM_ARGS();
	int filename_len;
	long flags = 0;
	FIBITMAP *bitmap = NULL;
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	php_stream *stream;
	FILE *fp = NULL;
	char *stream_buff = NULL;
	size_t stream_buff_size;
	FreeImageIO io;

	if (zend_parse_parameters(argc TSRMLS_CC, "s|l", &filename, &filename_len, &flags) == FAILURE) { 
		return;
	}
	stream = php_stream_open_wrapper(filename, "rb",
			REPORT_ERRORS|IGNORE_PATH|IGNORE_URL_WIN|ENFORCE_SAFE_MODE, NULL);
	if (stream == NULL)	{
		RETURN_FALSE;
	}
	// check if stream is stdio
	if (php_stream_is(stream, PHP_STREAM_IS_STDIO))	{
		// initialize file IO functions
		_php_freeimage_setio(&io, FREEIMAGEIO_FROM_FILE);
		if (FAILURE == php_stream_cast(stream, PHP_STREAM_AS_STDIO, (void**)&fp, REPORT_ERRORS)) {
			goto error_close;
		}
		// the third argument is currently not used by FreeImage
		fif = FreeImage_GetFileTypeFromHandle(&io, (fi_handle)fp, 0);
		if(fif == FIF_UNKNOWN) {
			// try to guess the file format from the file extension
			fif = FreeImage_GetFIFFromFilename(filename);
		}
		// check that the plugin has reading capabilities ...
		if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
			bitmap = (FIBITMAP *)emalloc(sizeof(FIBITMAP));
			// load from the handle
			bitmap = FreeImage_LoadFromHandle(fif, &io, (fi_handle)fp, flags);
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Doesn't support reading this image type");
			goto error_close;
		}
	} else {
		// initialize memory IO functions
		_php_freeimage_setio(&io, FREEIMAGEIO_FROM_MEMORY);
		// we can load from an memory io
		stream_buff_size = php_stream_copy_to_mem(stream, &stream_buff, PHP_STREAM_COPY_ALL, 1);

		if(!stream_buff_size) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot read image data");
			goto error_close;
		}
		freeimageio_memptr = stream_buff;
		// the third argument is currently not used by FreeImage
		fif = FreeImage_GetFileTypeFromHandle(&io, stream_buff, 0);
		if(fif == FIF_UNKNOWN) {
			// try to guess the file format from the file extension
			fif = FreeImage_GetFIFFromFilename(filename);
		}
		// check that the plugin has reading capabilities ...
		if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
			bitmap = (FIBITMAP *)emalloc(sizeof(FIBITMAP));
			// load from the handle
			bitmap = FreeImage_LoadFromHandle(fif, &io, (fi_handle)stream_buff, flags);
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Doesn't support reading this image type");
			goto error_close;
		}
	}
	if (bitmap != NULL) {
		ZEND_REGISTER_RESOURCE(return_value, bitmap, le_freeimage_bitmap);
		php_stream_close(stream);
		return;
	}

	php_error_docref(NULL TSRMLS_CC, E_WARNING, "'%s' is not a valid file", filename);
error_close:
	php_stream_close(stream);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool freeimage_save(resource bitmap, string filename [, int flags])
   Saves a previously loaded FIBITMAP to a file. */
PHP_FUNCTION(freeimage_save)
{
	char *filename = NULL;
	int argc = ZEND_NUM_ARGS();
	
	int filename_len;
	long flags = 0;
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL;
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	unsigned bpp;

	if (zend_parse_parameters(argc TSRMLS_CC, "rs|l", &z_bitmap, &filename, &filename_len, &flags) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);

	if (!filename || filename == "" || php_check_open_basedir(filename TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid filename '%s'", filename);
		RETURN_FALSE;
	}
	if (bitmap) {
		// try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilename(filename);
		// check that the plugin has writing capabilities ...
		if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsWriting(fif)) {
			bpp = FreeImage_GetBPP(bitmap);
			if (fif == FIF_JPEG && (bpp != 24 && bpp != 8)) {
				// Convert JPEG to 24 bits before saving
				bitmap = FreeImage_ConvertTo24Bits(bitmap);
			} else if (fif == FIF_GIF && bpp != 8) {
				// Convert GIF to 8 bits before saving
				bitmap = FreeImage_ConvertTo8Bits(bitmap);
			}
			if (FreeImage_Save(fif, bitmap, filename, flags)) {
				RETURN_TRUE;
			} else {
				VCWD_UNLINK(filename);
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_Save failed");
			}
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Doesn't support writing this image type");
		}
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto void freeimage_unload(resource bitmap)
   Deletes a previously loaded FIBITMAP from memory. */
PHP_FUNCTION(freeimage_unload)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	zend_list_delete(Z_RESVAL_P(z_bitmap));
}
/* }}} */

/* {{{ proto resource freeimage_clone(resource bitmap)
   Makes an exact reproduction of an existing bitmap. */
PHP_FUNCTION(freeimage_clone)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL, *clone_bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	clone_bitmap = (FIBITMAP *)emalloc(sizeof(FIBITMAP));
	clone_bitmap = (FIBITMAP *)FreeImage_Clone(bitmap);
	if (clone_bitmap != NULL) {
		ZEND_REGISTER_RESOURCE(return_value, clone_bitmap, le_freeimage_bitmap);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_Clone failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto string freeimage_getversion()
   Returns a string containing the current version of the DLL. */
PHP_FUNCTION(freeimage_getversion)
{
	char *version = NULL;
	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}
	version = (char *)FreeImage_GetVersion();
	RETURN_STRINGL(version, strlen(version), 1);
}
/* }}} */

/* {{{ proto string freeimage_getcopyrightmessage()
   Returns a string containing a standard copyright message. */
PHP_FUNCTION(freeimage_getcopyrightmessage)
{
	char *copyright;
	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}
	copyright = (char *)FreeImage_GetCopyrightMessage();
	RETURN_STRINGL(copyright, strlen(copyright), 1);
}
/* }}} */

/* {{{ proto string freeimage_getlasterror()
   Returns the last error message. */
PHP_FUNCTION(freeimage_getlasterror)
{
	char *error_msg;
	int error_len = 1;
	error_msg = estrdup("");
	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	if (FREEIMAGE_G(freeimage_lasterror)) {
		//php_printf("\n***\n%s\n***\n", FREEIMAGE_G(freeimage_lasterror));
		error_len = strlen(FREEIMAGE_G(freeimage_lasterror)) + 1;
		error_msg = (char *)erealloc(error_msg, error_len);
		if (!error_msg) {
			php_printf("Couldn't allocate memery!");
			return;
		}
		strcat(error_msg, FREEIMAGE_G(freeimage_lasterror));
	}
	RETURN_STRINGL(error_msg, --error_len, 0);
	efree(error_msg);
}
/* }}} */

/* {{{ proto int freeimage_getimagetype(resource bitmap)
   Returns the data type of a bitmap. */
PHP_FUNCTION(freeimage_getimagetype)
{
	int argc = ZEND_NUM_ARGS();
	unsigned imagetype;
	
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	imagetype = FreeImage_GetImageType(bitmap);
	RETURN_LONG((long)imagetype);
}
/* }}} */

/* {{{ proto int freeimage_getcolorsused(resource bitmap)
   Returns the number of colors used in a bitmap. This function returns the palette-size for
   palletised bitmaps, and 0 for high-colour bitmaps. */
PHP_FUNCTION(freeimage_getcolorsused)
{
	int argc = ZEND_NUM_ARGS();
	unsigned colors;
	
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	colors = FreeImage_GetColorsUsed(bitmap);
	RETURN_LONG((long)colors);
}
/* }}} */

/* {{{ proto int freeimage_getbpp(resource bitmap)
   Returns the size of one pixel in the bitmap in bits.
   Possible bit depths are 1, 4, 8, 16, 24, 32, 64 and 128. */
PHP_FUNCTION(freeimage_getbpp)
{
	int argc = ZEND_NUM_ARGS();
	unsigned bpp;
	
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	bpp = FreeImage_GetBPP(bitmap);
	RETURN_LONG((long)bpp);
}
/* }}} */

/* {{{ proto int freeimage_getwidth(resource bitmap)
   Returns the width of the bitmap in pixels. */
PHP_FUNCTION(freeimage_getwidth)
{
	int argc = ZEND_NUM_ARGS();
	unsigned width;
	
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	width = FreeImage_GetWidth(bitmap);
	RETURN_LONG((long)width);
}
/* }}} */

/* {{{ proto int freeimage_getheight(resource bitmap)
   Returns the height of the bitmap in pixels. */
PHP_FUNCTION(freeimage_getheight)
{
	int argc = ZEND_NUM_ARGS();
	unsigned height;
	
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	height = FreeImage_GetHeight(bitmap);
	RETURN_LONG((long)height);
}
/* }}} */

/* {{{ proto int freeimage_getline(resource bitmap)
   Returns the width of the bitmap in bytes. */
PHP_FUNCTION(freeimage_getline)
{
	int argc = ZEND_NUM_ARGS();
	unsigned line;
	
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	line = FreeImage_GetLine(bitmap);
	RETURN_LONG((long)line);
}
/* }}} */

/* {{{ proto int freeimage_getpitch(resource bitmap)
   Returns the width of the bitmap in bytes, rounded to the next 32-bit boundary, 
   also known as pitch or stride or scan width. */
PHP_FUNCTION(freeimage_getpitch)
{
	int argc = ZEND_NUM_ARGS();
	unsigned pitch;
	
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	pitch = FreeImage_GetPitch(bitmap);
	RETURN_LONG((long)pitch);
}
/* }}} */

/* {{{ proto int freeimage_getdibsize(resource bitmap)
   Returns the size of the DIB-element of a FIBITMAP in memory,
   i.e. the BITMAPINFOHEADER + palette + data bits. */
PHP_FUNCTION(freeimage_getdibsize)
{
	int argc = ZEND_NUM_ARGS();
	unsigned size;
	
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	size = FreeImage_GetDIBSize(bitmap);
	RETURN_LONG((long)size);
}
/* }}} */

/* {{{ proto int freeimage_getfiletype(string filename [, int size])
   Returns one of the predefined FREE_IMAGE_FORMAT constants or a bitmap identification 
   number registered by a plugin. */
PHP_FUNCTION(freeimage_getfiletype)
{
	char *filename = NULL;
	int argc = ZEND_NUM_ARGS();
	int filename_len;
	long size;
	FREE_IMAGE_FORMAT fif;

	if (zend_parse_parameters(argc TSRMLS_CC, "s|l", &filename, &filename_len, &size) == FAILURE) 
		return;
	size = 0; //The size parameter is currently not used and can be set to 0.
	fif = FreeImage_GetFileType(filename, size);
	RETURN_LONG((long)fif);
}
/* }}} */

/* {{{ proto int freeimage_getfifcount()
   Retrieves the number of FREE_IMAGE_FORMAT identifiers being currently registered. */
PHP_FUNCTION(freeimage_getfifcount)
{
	int count = 0;
	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}
	count = FreeImage_GetFIFCount();
	RETURN_LONG((long)count);
}
/* }}} */

/* {{{ proto int freeimage_setpluginenabled(int fif [, bool enable])
   Enables or disables a plugin. */
PHP_FUNCTION(freeimage_setpluginenabled)
{
	int argc = ZEND_NUM_ARGS();
	long fif;
	zend_bool enable = TRUE;
	int state;

	if (zend_parse_parameters(argc TSRMLS_CC, "l|b", &fif, &enable) == FAILURE) 
		return;
	state = FreeImage_SetPluginEnabled((FREE_IMAGE_FORMAT)fif, enable);
	RETURN_LONG((long)state);
}
/* }}} */

/* {{{ proto bool freeimage_ispluginenabled(int fif)
   Enables or disables a plugin. */
PHP_FUNCTION(freeimage_ispluginenabled)
{
	int argc = ZEND_NUM_ARGS();
	long fif;
	int state;

	if (zend_parse_parameters(argc TSRMLS_CC, "l", &fif) == FAILURE) 
		return;
	state = FreeImage_IsPluginEnabled((FREE_IMAGE_FORMAT)fif);
	RETURN_BOOL((long)state);
}
/* }}} */

/* {{{ proto int freeimage_getfiffromformat(string format)
   Returns a FREE_IMAGE_FORMAT identifier from the format string that was used to register the FIF. */
PHP_FUNCTION(freeimage_getfiffromformat)
{
	char *format = NULL;
	int argc = ZEND_NUM_ARGS();
	int format_len;
	FREE_IMAGE_FORMAT fif;

	if (zend_parse_parameters(argc TSRMLS_CC, "s", &format, &format_len) == FAILURE) 
		return;
	fif = FreeImage_GetFIFFromFormat(format);
	RETURN_LONG((long)fif);
}
/* }}} */

/* {{{ proto int freeimage_getfiffrommime(string mime)
   Returns a FREE_IMAGE_FORMAT identifier from a MIME content type string 
   (MIME stands for Multipurpose Internet Mail Extension). */
PHP_FUNCTION(freeimage_getfiffrommime)
{
	char *mime = NULL;
	int argc = ZEND_NUM_ARGS();
	int mime_len;
	FREE_IMAGE_FORMAT fif;

	if (zend_parse_parameters(argc TSRMLS_CC, "s", &mime, &mime_len) == FAILURE) 
		return;
	fif = FreeImage_GetFIFFromMime(mime);
	RETURN_LONG((long)fif);
}
/* }}} */

/* {{{ proto string freeimage_getformatfromfif(int fif)
   Returns the string that was used to register a plugin from the system assigned FREE_IMAGE_FORMAT. */
PHP_FUNCTION(freeimage_getformatfromfif)
{
	int argc = ZEND_NUM_ARGS();
	long fif;
	char *format = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "l", &fif) == FAILURE) 
		return;
	format = (char *)FreeImage_GetFormatFromFIF((FREE_IMAGE_FORMAT)fif);
	RETURN_STRINGL(format, strlen(format), 1);
}
/* }}} */

/* {{{ proto string freeimage_getfifextensionlist(int fif)
   Returns a comma-delimited file extension list describing the bitmap formats the given plugin 
   can read and/or write. */
PHP_FUNCTION(freeimage_getfifextensionlist)
{
	int argc = ZEND_NUM_ARGS();
	long fif;
	char *extension = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "l", &fif) == FAILURE) 
		return;
	extension = (char *)FreeImage_GetFIFExtensionList((FREE_IMAGE_FORMAT)fif);
	RETURN_STRINGL(extension, strlen(extension), 1);
}
/* }}} */

/* {{{ proto string freeimage_getfifdescription(int fif)
   Returns a descriptive string that describes the bitmap formats the given plugin can read and/or write. */
PHP_FUNCTION(freeimage_getfifdescription)
{
	int argc = ZEND_NUM_ARGS();
	long fif;
	char *description = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "l", &fif) == FAILURE) 
		return;
	description = (char *)FreeImage_GetFIFDescription((FREE_IMAGE_FORMAT)fif);
	RETURN_STRINGL(description, strlen(description), 1);
}
/* }}} */

/* {{{ proto string freeimage_getfifregexpr(int fif)
   Returns a regular expression string that can be used by a regular expression engine
   to identify the bitmap. */
PHP_FUNCTION(freeimage_getfifregexpr)
{
	int argc = ZEND_NUM_ARGS();
	long fif;
	char *regexpr = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "l", &fif) == FAILURE) 
		return;
	regexpr = (char *)FreeImage_GetFIFRegExpr((FREE_IMAGE_FORMAT)fif);
	RETURN_STRINGL(regexpr, strlen(regexpr), 1);
}
/* }}} */

/* {{{ proto int freeimage_getfiffromfilename(string filename)
   Returns the plugin that can read/write files with that extension in the form of 
   a FREE_IMAGE_FORMAT identifier. */
PHP_FUNCTION(freeimage_getfiffromfilename)
{
	char *filename = NULL;
	int argc = ZEND_NUM_ARGS();
	int filename_len;
	FREE_IMAGE_FORMAT fif;

	if (zend_parse_parameters(argc TSRMLS_CC, "s", &filename, &filename_len) == FAILURE) 
		return;
	fif = FreeImage_GetFIFFromFilename(filename);
	RETURN_LONG((long)fif);
}
/* }}} */

/* {{{ proto bool freeimage_fifsupportsreading(int fif)
   Returns TRUE if the plugin belonging to the given FREE_IMAGE_FORMAT can be used to 
   load bitmaps, FALSE otherwise. */
PHP_FUNCTION(freeimage_fifsupportsreading)
{
	int argc = ZEND_NUM_ARGS();
	long fif;

	if (zend_parse_parameters(argc TSRMLS_CC, "l", &fif) == FAILURE) 
		return;
	if (FreeImage_FIFSupportsReading((FREE_IMAGE_FORMAT)fif)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool freeimage_fifsupportswriting(int fif)
   Returns TRUE if the plugin belonging to the given FREE_IMAGE_FORMAT can be used to 
   save bitmaps, FALSE otherwise. */
PHP_FUNCTION(freeimage_fifsupportswriting)
{
	int argc = ZEND_NUM_ARGS();
	long fif;

	if (zend_parse_parameters(argc TSRMLS_CC, "l", &fif) == FAILURE) 
		return;
	if (FreeImage_FIFSupportsWriting((FREE_IMAGE_FORMAT)fif)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto resource freeimage_rotate(resource bitmap, double angle)
   Rotates an 8-bit greyscale, 24- or 32-bit image by means of 3 shears. */
PHP_FUNCTION(freeimage_rotate)
{
	int argc = ZEND_NUM_ARGS();
	
	double angle;
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL, *rotate_bitmap = NULL;
	unsigned bpp;

	if (zend_parse_parameters(argc TSRMLS_CC, "rd", &z_bitmap, &angle) == FAILURE) 
		return;
	angle = (angle <0 || angle > 360) ? 0 : angle;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	bpp = FreeImage_GetBPP(bitmap);
	// Convert to 24 bits if necessary
	if (bpp != 8 && bpp != 24 && bpp != 32) {
		bitmap = FreeImage_ConvertTo24Bits(bitmap);
	}
	rotate_bitmap = (FIBITMAP *)emalloc(sizeof(FIBITMAP));
	rotate_bitmap = FreeImage_RotateClassic(bitmap, angle);
	if (rotate_bitmap != NULL) {
		ZEND_REGISTER_RESOURCE(return_value, rotate_bitmap, le_freeimage_bitmap);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_RatateClassic failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto resource freeimage_rotateex(resource bitmap, double angle, double x_shift, double y_shift, double x_origin, double y_origin, bool use_mask)
   Performs a rotation and / or translation of an 8-bit greyscale, 24- or 32-bit image, 
   using a 3rd order (cubic) B-Spline. */
PHP_FUNCTION(freeimage_rotateex)
{
	int argc = ZEND_NUM_ARGS();
	
	double angle;
	double x_shift;
	double y_shift;
	double x_origin;
	double y_origin;
	zend_bool use_mask;
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL, *rotate_bitmap = NULL;
	unsigned bpp;

	if (zend_parse_parameters(argc TSRMLS_CC, "rdddddb", &z_bitmap, &angle, &x_shift, &y_shift, &x_origin, &y_origin, &use_mask) == FAILURE) 
		return;
	angle		= (angle < 0 || angle > 360) ? 0 : angle;
	x_shift		= (x_shift < 0) ? 0 : x_shift;
	y_shift		= (y_shift < 0) ? 0 : y_shift;
	x_origin	= (x_origin < 0) ? 0 : x_origin;
	y_origin	= (y_origin < 0) ? 0 : y_origin;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	bpp = FreeImage_GetBPP(bitmap);
	// Convert to 24 bits if necessary
	if (bpp != 8 && bpp != 24 && bpp != 32) {
		bitmap = FreeImage_ConvertTo24Bits(bitmap);
	}
	rotate_bitmap = (FIBITMAP *)emalloc(sizeof(FIBITMAP));
	rotate_bitmap = FreeImage_RotateEx(bitmap, angle, x_shift, y_shift, x_origin, y_origin, use_mask);
	if (rotate_bitmap != NULL) {
		ZEND_REGISTER_RESOURCE(return_value, rotate_bitmap, le_freeimage_bitmap);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_RotateEx failed");
		RETURN_FALSE;
	}	
}
/* }}} */

/* {{{ proto bool freeimage_fliphorizontal(resource bitmap)
   Flip the input bitmap horizontally along the vertical axis. */
PHP_FUNCTION(freeimage_fliphorizontal)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	if (FreeImage_FlipHorizontal(bitmap)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool freeimage_flipvertical(resource bitmap)
   Flip the input bitmap vertically along the horizontal axis. */
PHP_FUNCTION(freeimage_flipvertical)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	if (FreeImage_FlipVertical(bitmap)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto mixed freeimage_rescale(resource bitmap, int dst_width, int dst_height, int filter)
   Performs resampling (or scaling, zooming) of a 8-, 24- or 32-bit image to 
   the desired destination width and height. */
PHP_FUNCTION(freeimage_rescale)
{
	int argc = ZEND_NUM_ARGS();
	
	long dst_width;
	long dst_height;
	long filter;
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL, *rescale_bitmap = NULL;
	unsigned bpp;

	if (zend_parse_parameters(argc TSRMLS_CC, "rlll", &z_bitmap, &dst_width, &dst_height, &filter) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	bpp = FreeImage_GetBPP(bitmap);
	// Convert to 24 bits if necessary
	if (bpp != 8 && bpp != 24 && bpp != 32) {
		bitmap = FreeImage_ConvertTo24Bits(bitmap);
	}
	rescale_bitmap = (FIBITMAP *)emalloc(sizeof(FIBITMAP));
	rescale_bitmap = FreeImage_Rescale(bitmap, dst_width, dst_height, (FREE_IMAGE_FILTER)filter);
	if (rescale_bitmap != NULL) {
		ZEND_REGISTER_RESOURCE(return_value, rescale_bitmap, le_freeimage_bitmap);
	} else {
		if (bitmap != NULL) {
			FreeImage_Unload(bitmap);
		}
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_Rescale failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool freeimage_gamma(resource bitmap, double gamma)
   Performs gamma correction on a 8-, 24- or 32-bit image. 
   The function returns TRUE on success. 
   It returns FALSE when gamma is less than or equal to zero or 
   when the bitdepth of the source bitmap cannot be handled. */
PHP_FUNCTION(freeimage_gamma)
{
	int argc = ZEND_NUM_ARGS();
	
	double gamma;
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL;
	unsigned bpp;

	if (zend_parse_parameters(argc TSRMLS_CC, "rd", &z_bitmap, &gamma) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	bpp = FreeImage_GetBPP(bitmap);
	// Convert to 24 bits if necessary
	if (bpp != 8 && bpp != 24 && bpp != 32) {
		bitmap = FreeImage_ConvertTo24Bits(bitmap);
	}
	if (FreeImage_AdjustGamma(bitmap, gamma)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool freeimage_brightness(resource bitmap, double percentage)
   Adjusts the brightness of a 8-, 24- or 32-bit image by a certain amount. 
   This amount is given by the percentage parameter, where percentage is a value between [-100..100]. 
   A value 0 means no change, less than 0 will make the image darker and 
   greater than 0 will make the image brighter. 
   The function returns TRUE on success, FALSE otherwise 
   (e.g. when the bitdepth of the source bitmap cannot be handled). */
PHP_FUNCTION(freeimage_brightness)
{
	int argc = ZEND_NUM_ARGS();
	
	double percentage;
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL;
	unsigned bpp;

	if (zend_parse_parameters(argc TSRMLS_CC, "rd", &z_bitmap, &percentage) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	bpp = FreeImage_GetBPP(bitmap);
	// Convert to 24 bits if necessary
	if (bpp != 8 && bpp != 24 && bpp != 32) {
		bitmap = FreeImage_ConvertTo24Bits(bitmap);
	}
	if (FreeImage_AdjustBrightness(bitmap, percentage)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool freeimage_contrast(resource bitmap, double percentage)
   Adjusts the contrast of a 8-, 24- or 32-bit image by a certain amount. 
   This amount is given by the percentage parameter, where percentage is a value between [-100..100]. 
   A value 0 means no change, less than 0 will decrease the contrast and 
   greater than 0 will increase the contrast of the image. 
   The function returns TRUE on success, FALSE otherwise 
   (e.g. when the bitdepth of the source bitmap cannot be handled). */
PHP_FUNCTION(freeimage_contrast)
{
	int argc = ZEND_NUM_ARGS();
	
	double percentage;
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL;
	unsigned bpp;

	if (zend_parse_parameters(argc TSRMLS_CC, "rd", &z_bitmap, &percentage) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	bpp = FreeImage_GetBPP(bitmap);
	// Convert to 24 bits if necessary
	if (bpp != 8 && bpp != 24 && bpp != 32) {
		bitmap = FreeImage_ConvertTo24Bits(bitmap);
	}
	if (FreeImage_AdjustContrast(bitmap, percentage)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool freeimage_invert(resource bitmap)
   Inverts each pixel data. */
PHP_FUNCTION(freeimage_invert)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	if (FreeImage_Invert(bitmap)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto mixed freeimage_copy(resource bitmap, int left, int top, int right, int bottom)
   Copy a sub part of the current bitmap image and returns it as a FIBITMAP*. 
   The function returns the subimage if successful, NULL otherwise. */
PHP_FUNCTION(freeimage_copy)
{
	int argc = ZEND_NUM_ARGS();
	long left;
	long top;
	long right;
	long bottom;
	
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL, *copy_bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "rllll", &z_bitmap, &left, &top, &right, &bottom) == FAILURE) 
		return;
	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	copy_bitmap = (FIBITMAP *)emalloc(sizeof(FIBITMAP));
	copy_bitmap = FreeImage_Copy(bitmap, left, top, right, bottom);
	if (copy_bitmap != NULL) {
		ZEND_REGISTER_RESOURCE(return_value, copy_bitmap, le_freeimage_bitmap);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_Copy failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto mixed freeimage_paste(resource dst, resource src, int left, int top, int alpha)
   Alpha blend or combine a sub part image with the current bitmap image. 
   The function returns TRUE if successful, FALSE otherwise. */
PHP_FUNCTION(freeimage_paste)
{
	int argc = ZEND_NUM_ARGS();
	long left;
	long top;
	long alpha;
	zval *z_dst = NULL;
	zval *z_src = NULL;
	FIBITMAP *dst_bitmap = NULL, *src_bitmap = NULL;
	unsigned bpp_dst, bpp_src;

	if (zend_parse_parameters(argc TSRMLS_CC, "rrlll", &z_dst, &z_src, &left, &top, &alpha) == FAILURE) 
		return;
	if (alpha <0 || alpha > 256) {
		alpha = 255;
	}

	ZEND_FETCH_RESOURCE(dst_bitmap, FIBITMAP *, &z_dst, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	ZEND_FETCH_RESOURCE(src_bitmap, FIBITMAP *, &z_src, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	
	bpp_dst = FreeImage_GetBPP(dst_bitmap);
	bpp_src = FreeImage_GetBPP(src_bitmap);
	if (bpp_src == 8 || bpp_src == 16 || bpp_src == 24 || bpp_src == 32) {
		if (bpp_dst < bpp_src) {
			switch (bpp_src) {
				case 8:
					dst_bitmap = FreeImage_ConvertTo8Bits(dst_bitmap);
					break;
				case 16:
					dst_bitmap = FreeImage_ConvertTo16Bits565(dst_bitmap);
					break;
				case 24:
					dst_bitmap = FreeImage_ConvertTo24Bits(dst_bitmap);
					break;
				default:
				case 32:
					dst_bitmap = FreeImage_ConvertTo32Bits(dst_bitmap);
					break;
			}
		}
	} else {
		src_bitmap = FreeImage_ConvertTo24Bits(src_bitmap);
		if (bpp_dst < 24) {
			dst_bitmap = FreeImage_ConvertTo24Bits(dst_bitmap);
		}
	}
	if (FreeImage_Paste(dst_bitmap, src_bitmap, left, top, alpha)) {
		RETURN_TRUE;
	} else {
		if (dst_bitmap != NULL) {
			FreeImage_Unload(dst_bitmap);
		}
		if (src_bitmap != NULL) {
			FreeImage_Unload(src_bitmap);
		}
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto resource freeimage_composite(resource fg [, bool usefilebkg [, mixed bkcolor [, resource bg]]])
   This function composite a foreground image against a single background color or 
   against a background image. */
PHP_FUNCTION(freeimage_composite)
{
	/*
	int argc = ZEND_NUM_ARGS();
	zend_bool usefilebkg = FALSE;
	zval *z_fg = NULL;
	zval *bkcolor = NULL;
	zval *z_bg = NULL;
	FIBITMAP *fg_bitmap = NULL, *bg_bitmap = NULL, *new_bitmap = NULL;
	unsigned bpp_fg, bpp_bg;

	if (zend_parse_parameters(argc TSRMLS_CC, "r|bzr", &z_fg, &usefilebkg, &bkcolor, &z_bg) == FAILURE) 
		return;
	ZEND_FETCH_RESOURCE(fg_bitmap, FIBITMAP *, &z_fg, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	bpp_fg = FreeImage_GetBPP(fg_bitmap);
	if (bpp_fg != 8 && bpp_fg != 32) {
		fg_bitmap = FreeImage_ConvertTo32Bits(fg_bitmap);
		bpp_fg = 32;
	}
	new_bitmap = (FIBITMAP *)emalloc(sizeof(FIBITMAP));
	switch(argc) {
		case 1:
			// use a checkerboard background
			//new_bitmap = FreeImage_Composite(fg_bitmap);
			break;
		case 2:
			// use the image file background if there is one
			//new_bitmap = FreeImage_Composite(fg_bitmap, usefilebkg);
			break;
		case 3:
			// use a user specified background
			if (bkcolor == NULL) {
				//new_bitmap = FreeImage_Composite(fg_bitmap, FALSE, NULL);
			} else {
				RGBQUAD appColor = {0, 255, 0, 0};
				//new_bitmap = FreeImage_Composite(fg_bitmap, FALSE, &appColor);
			}
			break;
		case 4:
			// use bg as the background image
			// bg MUST BE a 24-bit image with the same width and height as fg
			ZEND_FETCH_RESOURCE(bg_bitmap, FIBITMAP *, &z_bg, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
			bpp_bg = FreeImage_GetBPP(bg_bitmap);
			if (bpp_bg != 8 && bpp_bg != 32) {
				if (bpp_fg == 8) {
					bg_bitmap = FreeImage_ConvertTo8Bits(bg_bitmap);
				} else {
					bg_bitmap = FreeImage_ConvertTo32Bits(bg_bitmap);
				}
			} else {
				if (bpp_fg != bpp_bg) {
					// todo ...
				}
			}
			//new_bitmap = FreeImage_Composite(fg_bitmap, FALSE, NULL, bg_bitmap);
			break;
	}
	if (new_bitmap != NULL) {
		ZEND_REGISTER_RESOURCE(return_value, new_bitmap, le_freeimage_bitmap);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_Composite failed");
		RETURN_FALSE;
	}
	*/
	php_error(E_WARNING, "FreeImage_GetLockedPageNumbers: not yet implemented");
}
/* }}} */

/* {{{ proto resource freeimage_convertto8bits(resource bitmap)
   Converts a bitmap to 8 bits. If the bitmap was a high-color bitmap (16, 24 or 32-bit) or 
   if it was a monochrome or greyscale bitmap (1 or 4-bit), 
   the end result will be a greyscale bitmap, 
   otherwise (1 or 4-bit palletised bitmaps) it will be a palletised bitmap. */
PHP_FUNCTION(freeimage_convertto8bits)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL, *conv_bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	conv_bitmap = (FIBITMAP *)emalloc(sizeof(FIBITMAP));
	conv_bitmap = FreeImage_ConvertTo8Bits(bitmap);
	if (conv_bitmap != NULL) {
		ZEND_REGISTER_RESOURCE(return_value, conv_bitmap, le_freeimage_bitmap);
	} else {
		if (bitmap != NULL) {
			FreeImage_Unload(bitmap);
		}
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_ConvertTo8Bits failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto resource freeimage_convertto16bits555(resource bitmap)
   Converts a bitmap to 16 bits, where each pixel has a color pattern of 
   5 bits red, 5 bits green and 5 bits blue. One bit in each pixel is unused. */
PHP_FUNCTION(freeimage_convertto16bits555)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL, *conv_bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	conv_bitmap = (FIBITMAP *)emalloc(sizeof(FIBITMAP));
	conv_bitmap = FreeImage_ConvertTo16Bits555(bitmap);
	if (conv_bitmap != NULL) {
		ZEND_REGISTER_RESOURCE(return_value, conv_bitmap, le_freeimage_bitmap);
	} else {
		if (bitmap != NULL) {
			FreeImage_Unload(bitmap);
		}
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_ConvertTo16Bits555 failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto resource freeimage_convertto16bits565(resource bitmap)
   Converts a bitmap to 16 bits, where each pixel has a color pattern of 
   5 bits red, 6 bits green and 5 bits blue. */
PHP_FUNCTION(freeimage_convertto16bits565)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL, *conv_bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	conv_bitmap = (FIBITMAP *)emalloc(sizeof(FIBITMAP));
	conv_bitmap = FreeImage_ConvertTo16Bits565(bitmap);
	if (conv_bitmap != NULL) {
		ZEND_REGISTER_RESOURCE(return_value, conv_bitmap, le_freeimage_bitmap);
	} else {
		if (bitmap != NULL) {
			FreeImage_Unload(bitmap);
		}
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_ConvertTo16Bits565 failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto resource freeimage_convertto24bits(resource bitmap)
   Converts a bitmap to 24 bits. */
PHP_FUNCTION(freeimage_convertto24bits)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL, *conv_bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	conv_bitmap = (FIBITMAP *)emalloc(sizeof(FIBITMAP));
	conv_bitmap = FreeImage_ConvertTo24Bits(bitmap);
	if (conv_bitmap != NULL) {
		ZEND_REGISTER_RESOURCE(return_value, conv_bitmap, le_freeimage_bitmap);
	} else {
		if (bitmap != NULL) {
			FreeImage_Unload(bitmap);
		}
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_ConvertTo24Bits failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto resource freeimage_convertto32bits(resource bitmap)
   Converts a bitmap to 32 bits. */
PHP_FUNCTION(freeimage_convertto32bits)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL, *conv_bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	conv_bitmap = (FIBITMAP *)emalloc(sizeof(FIBITMAP));
	conv_bitmap = FreeImage_ConvertTo32Bits(bitmap);
	if (conv_bitmap != NULL) {
		ZEND_REGISTER_RESOURCE(return_value, conv_bitmap, le_freeimage_bitmap);
	} else {
		if (bitmap != NULL) {
			FreeImage_Unload(bitmap);
		}
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_ConvertTo32Bits failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto resource freeimage_colorquantize(resource bitmap, int quantize)
   Quantizes a high-color 24-bit bitmap to an 8-bit palette color bitmap. */
PHP_FUNCTION(freeimage_colorquantize)
{
	int argc = ZEND_NUM_ARGS();
	
	long quantize;
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL, *conv_bitmap = NULL;
	unsigned bpp;

	if (zend_parse_parameters(argc TSRMLS_CC, "rl", &z_bitmap, &quantize) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	conv_bitmap = (FIBITMAP *)emalloc(sizeof(FIBITMAP));
	bpp = FreeImage_GetBPP(bitmap);
	if (bpp != 24) {
		// Convert to 24 bits if necessary
		bitmap = FreeImage_ConvertTo24Bits(bitmap);
	}
	conv_bitmap = FreeImage_ColorQuantize(bitmap, (FREE_IMAGE_QUANTIZE)quantize);
	if (conv_bitmap != NULL) {
		ZEND_REGISTER_RESOURCE(return_value, conv_bitmap, le_freeimage_bitmap);
	} else {
		if (bitmap != NULL) {
			FreeImage_Unload(bitmap);
		}
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_ColorQuantize failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto resource freeimage_threshold(resource bitmap, int threshold)
   Converts a bitmap to 1-bit monochrome bitmap using a threshold T between [0..255]. */
PHP_FUNCTION(freeimage_threshold)
{
	int argc = ZEND_NUM_ARGS();
	
	long threshold;
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL, *conv_bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "rl", &z_bitmap, &threshold) == FAILURE) 
		return;
	if (threshold < 0 || threshold > 255) {
		threshold = 128;
	}

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	conv_bitmap = (FIBITMAP *)emalloc(sizeof(FIBITMAP));
	conv_bitmap = FreeImage_Threshold(bitmap, (BYTE)threshold);
	if (conv_bitmap != NULL) {
		ZEND_REGISTER_RESOURCE(return_value, conv_bitmap, le_freeimage_bitmap);
	} else {
		if (bitmap != NULL) {
			FreeImage_Unload(bitmap);
		}
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_Threshold failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto resource freeimage_dither(resource bitmap, int algorithm)
   Converts a bitmap to 1-bit monochrome bitmap using a dithering algorithm. */
PHP_FUNCTION(freeimage_dither)
{
	int argc = ZEND_NUM_ARGS();
	
	long algorithm;
	zval *z_bitmap = NULL;
	FIBITMAP *bitmap = NULL, *conv_bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "rl", &z_bitmap, &algorithm) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	conv_bitmap = (FIBITMAP *)emalloc(sizeof(FIBITMAP));
	conv_bitmap = FreeImage_Dither(bitmap, (FREE_IMAGE_DITHER)algorithm);
	if (conv_bitmap != NULL) {
		ZEND_REGISTER_RESOURCE(return_value, conv_bitmap, le_freeimage_bitmap);
	} else {
		if (bitmap != NULL) {
			FreeImage_Unload(bitmap);
		}
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_Dither failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto resource freeimage_openmultibitmap(int fif, string filename, bool create_new, bool read_only [, bool keep_cache_in_memory])
   Opens a multi-paged bitmap.*/
PHP_FUNCTION(freeimage_openmultibitmap)
{
	char *filename = NULL;
	int argc = ZEND_NUM_ARGS();
	int filename_len;
	long fif;
	zend_bool create_new;
	zend_bool read_only;
	zend_bool keep_cache_in_memory = FALSE;
	FIMULTIBITMAP *multibitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "lsbb|b", &fif, &filename, &filename_len, &create_new, &read_only, &keep_cache_in_memory) == FAILURE) 
		return;
	multibitmap = (FIMULTIBITMAP *)emalloc(sizeof(FIMULTIBITMAP));
	multibitmap = FreeImage_OpenMultiBitmap((FREE_IMAGE_FORMAT)fif, filename, create_new, read_only, keep_cache_in_memory);
	if (multibitmap != NULL) {
		ZEND_REGISTER_RESOURCE(return_value, multibitmap, le_freeimage_multibitmap);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_OpenMultiBitmap failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool freeimage_closemultibitmap(resource multibitmap [, int flags])
   Closes a previously opened multi-page bitmap and, when the bitmap was not opened read-only, 
   applies any changes made to it.*/
PHP_FUNCTION(freeimage_closemultibitmap)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_multibitmap = NULL;
	long flags = 0;
	FIMULTIBITMAP *multibitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r|l", &z_multibitmap, &flags) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(multibitmap, FIMULTIBITMAP *, &z_multibitmap, -1, le_freeimage_name_multibitmap, le_freeimage_multibitmap);
	if (FreeImage_CloseMultiBitmap(multibitmap, flags)) {
		//zend_list_delete(Z_RESVAL_P(z_multibitmap));
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto int freeimage_getpagecount(resource multibitmap)
   Returns the number of pages currently available in the multi-paged bitmap.*/
PHP_FUNCTION(freeimage_getpagecount)
{
	int argc = ZEND_NUM_ARGS();
	zval *z_multibitmap = NULL;
	int pages;
	FIMULTIBITMAP *multibitmap = NULL;
	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_multibitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(multibitmap, FIMULTIBITMAP *, &z_multibitmap, -1, le_freeimage_name_multibitmap, le_freeimage_multibitmap);
	pages = FreeImage_GetPageCount(multibitmap);
	RETURN_LONG((long)pages);
}
/* }}} */

/* {{{ proto void freeimage_appendpage(resource multibitmap, resource bitmap)
   Appends a new page to the end of the bitmap.*/
PHP_FUNCTION(freeimage_appendpage)
{
	int argc = ZEND_NUM_ARGS();
	zval *z_multibitmap = NULL;
	zval *z_bitmap = NULL;
	FIMULTIBITMAP *multibitmap = NULL;
	FIBITMAP *bitmap = NULL;
	if (zend_parse_parameters(argc TSRMLS_CC, "rr", &z_multibitmap, &z_bitmap) == FAILURE) 
		return;
	ZEND_FETCH_RESOURCE(multibitmap, FIMULTIBITMAP *, &z_multibitmap, -1, le_freeimage_name_multibitmap, le_freeimage_multibitmap);
	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	FreeImage_AppendPage(multibitmap, bitmap);
}
/* }}} */

/* {{{ proto void freeimage_insertpage(resource multibitmap, int page, resource bitmap)
   Inserts a new page before the given position in the bitmap. 
   Page has to be a number smaller than the current number of pages available in the bitmap.*/
PHP_FUNCTION(freeimage_insertpage)
{
	int argc = ZEND_NUM_ARGS();
	zval *z_multibitmap = NULL;
	long page;
	zval *z_bitmap = NULL;
	FIMULTIBITMAP *multibitmap = NULL;
	FIBITMAP *bitmap = NULL;
	if (zend_parse_parameters(argc TSRMLS_CC, "rlr", &z_multibitmap, &page, &z_bitmap) == FAILURE) 
		return;
	ZEND_FETCH_RESOURCE(multibitmap, FIMULTIBITMAP *, &z_multibitmap, -1, le_freeimage_name_multibitmap, le_freeimage_multibitmap);
	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	FreeImage_InsertPage(multibitmap, page, bitmap);
}
/* }}} */

/* {{{ proto void freeimage_deletepage(resource multibitmap, int page)
   Deletes the page on the given position.*/
PHP_FUNCTION(freeimage_deletepage)
{
	int argc = ZEND_NUM_ARGS();
	zval *z_multibitmap = NULL;
	long page;
	FIMULTIBITMAP *multibitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "rl", &z_multibitmap, &page) == FAILURE) 
		return;
	ZEND_FETCH_RESOURCE(multibitmap, FIMULTIBITMAP *, &z_multibitmap, -1, le_freeimage_name_multibitmap, le_freeimage_multibitmap);
	FreeImage_DeletePage(multibitmap, page);
}
/* }}} */

/* {{{ proto resource freeimage_lockpage(resource multibitmap, int page)
   Locks a page in memory for editing. The page can now be saved to a different file or 
   inserted into another multi-page bitmap.*/
PHP_FUNCTION(freeimage_lockpage)
{
	int argc = ZEND_NUM_ARGS();
	zval *z_multibitmap = NULL;
	long page;
	FIMULTIBITMAP *multibitmap = NULL;
	FIBITMAP *lock_bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "rl", &z_multibitmap, &page) == FAILURE) 
		return;
	ZEND_FETCH_RESOURCE(multibitmap, FIMULTIBITMAP *, &z_multibitmap, -1, le_freeimage_name_multibitmap, le_freeimage_multibitmap);
	lock_bitmap = (FIBITMAP *)emalloc(sizeof(FIBITMAP));
	lock_bitmap = FreeImage_LockPage(multibitmap, (int)page);
	if (lock_bitmap) {
		ZEND_REGISTER_RESOURCE(return_value, lock_bitmap, le_freeimage_bitmap);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_LockPage failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto void freeimage_unlockpage(resource multibitmap, resource bitmap, bool changed)
   Unlocks a previously locked page and gives it back to the multi-page engine. 
   When the last parameter is TRUE, the page is marked changed and the new page data is applied 
   in the multi-page bitmap.*/
PHP_FUNCTION(freeimage_unlockpage)
{
	int argc = ZEND_NUM_ARGS();
	zval *z_multibitmap = NULL;
	zval *z_bitmap = NULL;
	zend_bool changed;
	FIMULTIBITMAP *multibitmap = NULL;
	FIBITMAP *bitmap = NULL;
	if (zend_parse_parameters(argc TSRMLS_CC, "rrb", &z_multibitmap, &z_bitmap, &changed) == FAILURE) 
		return;
	ZEND_FETCH_RESOURCE(multibitmap, FIMULTIBITMAP *, &z_multibitmap, -1, le_freeimage_name_multibitmap, le_freeimage_multibitmap);
	ZEND_FETCH_RESOURCE(bitmap, FIBITMAP *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	FreeImage_UnlockPage(multibitmap, bitmap, changed);
}
/* }}} */

/* {{{ proto bool freeimage_movepage(resource multibitmap, int target_page, int source_page)
   Moves the source page to the position of the target page. 
   Returns TRUE on success, FALSE on failure.*/
PHP_FUNCTION(freeimage_movepage)
{
	int argc = ZEND_NUM_ARGS();
	zval *z_multibitmap = NULL;
	long target_page;
	long source_page;
	FIMULTIBITMAP *multibitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "rll", &z_multibitmap, &target_page, &source_page) == FAILURE) 
		return;
	ZEND_FETCH_RESOURCE(multibitmap, FIMULTIBITMAP *, &z_multibitmap, -1, le_freeimage_name_multibitmap, le_freeimage_multibitmap);
	if (FreeImage_MovePage(multibitmap, (int)target_page, (int)source_page)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto int freeimage_getlockedpagenumbers(resource multibitmap, int pages, int count)
   Returns an array of page-numbers that are currently locked in memory. 
   When the pages parameter is NULL, the size of the array is returned in the count variable. 
   You can then allocate the array of the desired size and 
   call FreeImage_GetLockedPageNumbers again to populate the array.*/
PHP_FUNCTION(freeimage_getlockedpagenumbers)
{
	int argc = ZEND_NUM_ARGS();
	zval *z_multibitmap = NULL;
	long pages;
	long count;
	FIMULTIBITMAP *multibitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "rll", &z_multibitmap, &pages, &count) == FAILURE) 
		return;
	/*
	ZEND_FETCH_RESOURCE(multibitmap, FIMULTIBITMAP *, &z_multibitmap, -1, le_freeimage_name_multibitmap, le_freeimage_multibitmap);
	if (FreeImage_GetLockedPageNumbers(multibitmap, &pages, &count)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
	*/
	php_error(E_WARNING, "FreeImage_GetLockedPageNumbers: not yet implemented");
}
/* }}} */



/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
