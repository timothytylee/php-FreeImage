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

typedef struct {
	FIBITMAP *bitmap;
	zval *z_multi;		// Non-zero for locked pages of a multibitmap
    zend_bool changed;	// TRUE if marked as changed by UnlockPage()
} BITMAP_REC;

typedef struct {
	FITAG *tag;
    int allocated;		// Non-zero if tag needs to destroyed
} TAG_REC;

typedef struct {
	FIMETADATA *metadata;
	FITAG *tag;			// Tag from FreeImage_FindFirstMetadata or FreeImage_FindNextMetadata
} METADATA_REC;

ZEND_DECLARE_MODULE_GLOBALS(freeimage)

/* True global resources - no need for thread safety here */
static int le_freeimage_bitmap;
static int le_freeimage_multibitmap;
static int le_freeimage_tag;
static int le_freeimage_metadata;
#define le_freeimage_name_bitmap		"FreeImage Bitmap Handle"
#define le_freeimage_name_multibitmap	"FreeImage Multipage Bitmap Handle"
#define le_freeimage_name_tag			"FreeImage Tag Handle"
#define le_freeimage_name_metadata		"FreeImage Metadata Search Handle"

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

/* {{{ _php_freeimage_long_to_rgbquad
 */
static void _php_freeimage_long_to_rgbquad(long rgb, RGBQUAD* quad) {
	quad->rgbRed   = (rgb >> 0) & 0xff;
	quad->rgbGreen = (rgb >> 8) & 0xff;
	quad->rgbBlue  = (rgb >> 16) & 0xff;
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
zend_function_entry freeimage_functions[] = {
	/* Support for RGBQUAD */
	PHP_FE(freeimage_rgbquad,				NULL)
	/* Bitmap management functions */
	PHP_FE(freeimage_load,					NULL)
	PHP_FE(freeimage_save,					NULL)
	PHP_FE(freeimage_unload,				NULL)
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
	/* Metadata functions */
	PHP_FE(freeimage_createtag,				NULL)
	PHP_FE(freeimage_deletetag,				NULL)
	PHP_FE(freeimage_clonetag,				NULL)
	PHP_FE(freeimage_gettagkey,				NULL)
	PHP_FE(freeimage_gettagdescription,		NULL)
	PHP_FE(freeimage_gettagid,				NULL)
	PHP_FE(freeimage_gettagtype,			NULL)
	PHP_FE(freeimage_gettagcount,			NULL)
	PHP_FE(freeimage_gettagvalue,			NULL)
	PHP_FE(freeimage_settagkey,				NULL)
	PHP_FE(freeimage_settagdescription,		NULL)
	PHP_FE(freeimage_settagid,				NULL)
	PHP_FE(freeimage_settagtype,			NULL)
	PHP_FE(freeimage_settagcount,			NULL)
	PHP_FE(freeimage_settagvalue,			NULL)
	PHP_FE(freeimage_findfirstmetadata,		NULL)
	PHP_FE(freeimage_findnextmetadata,		NULL)
	PHP_FE(freeimage_findclosemetadata,		NULL)
	PHP_FE(freeimage_findtag,				NULL)
	PHP_FE(freeimage_getmetadata,			NULL)
	PHP_FE(freeimage_setmetadata,			NULL)
	PHP_FE(freeimage_getmetadatacount,		NULL)
	PHP_FE(freeimage_tagtostring,			NULL)
	{NULL, NULL, NULL}	/* Must be the last line in freeimage_functions[] */
};
/* }}} */

/* {{{ freeimage_class_functions[]
 */
zend_function_entry freeimage_class_functions[] = {
	PHP_FALIAS(freeimage,			freeimage_constructor,			NULL)
	/* Support for RGBQUAD */
	PHP_FALIAS(rgbquad,				freeimage_rgbquad,				NULL)
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
	/* Metadata functions */
	PHP_FALIAS(createtag,			freeimage_createtag,			NULL)
	PHP_FALIAS(deletetag,			freeimage_deletetag,			NULL)
	PHP_FALIAS(clonetag,			freeimage_clonetag,				NULL)
	PHP_FALIAS(gettagkey,			freeimage_gettagkey,			NULL)
	PHP_FALIAS(gettagdescription,	freeimage_gettagdescription,	NULL)
	PHP_FALIAS(gettagid,			freeimage_gettagid,				NULL)
	PHP_FALIAS(gettagtype,			freeimage_gettagtype,			NULL)
	PHP_FALIAS(gettagcount,			freeimage_gettagcount,			NULL)
	PHP_FALIAS(gettagvalue,			freeimage_gettagvalue,			NULL)
	PHP_FALIAS(settagkey,			freeimage_settagkey,			NULL)
	PHP_FALIAS(settagdescription,	freeimage_settagdescription,	NULL)
	PHP_FALIAS(settagid,			freeimage_settagid,				NULL)
	PHP_FALIAS(settagtype,			freeimage_settagtype,			NULL)
	PHP_FALIAS(settagcount,			freeimage_settagcount,			NULL)
	PHP_FALIAS(settagvalue,			freeimage_settagvalue,			NULL)
	PHP_FALIAS(findfirstmetadata,	freeimage_findfirstmetadata,	NULL)
	PHP_FALIAS(findnextmetadata,	freeimage_findnextmetadata,		NULL)
	PHP_FALIAS(findclosemetadata,	freeimage_findclosemetadata,	NULL)
	PHP_FALIAS(findtag,				freeimage_findtag,				NULL)
	PHP_FALIAS(getmetadata,			freeimage_getmetadata,			NULL)
	PHP_FALIAS(setmetadata,			freeimage_setmetadata,			NULL)
	PHP_FALIAS(getmetadatacount,	freeimage_getmetadatacount,		NULL)
	PHP_FALIAS(tagtostring,			freeimage_tagtostring,			NULL)
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
	BITMAP_REC *rec = (BITMAP_REC *)rsrc->ptr;
	if (rec->bitmap)
	{
		if (rec->z_multi)
		{
			FIMULTIBITMAP *multibitmap = NULL;
			multibitmap = (FIMULTIBITMAP *)zend_fetch_resource(
				&rec->z_multi TSRMLS_CC, -1, le_freeimage_name_multibitmap,
				NULL, 1, le_freeimage_multibitmap);
			if (multibitmap)
			{
				FreeImage_UnlockPage(multibitmap, rec->bitmap, rec->changed);
				zend_list_delete(Z_RESVAL_P(rec->z_multi));
			}
		}
		else
		{
			FreeImage_Unload(rec->bitmap);
		}
	}
	efree(rec);
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

/* {{{ _php_freeimage_tag_dtor
 */
static void _php_freeimage_tag_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	TAG_REC *rec = (TAG_REC *)rsrc->ptr;
	if (rec->allocated) {
		FreeImage_DeleteTag(rec->tag);
	}
	efree(rec);
}
/* }}} */

/* {{{ _php_freeimage_metadata_dtor
 */
static void _php_freeimage_metadata_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	METADATA_REC *rec = (METADATA_REC *)rsrc->ptr;
	FreeImage_FindCloseMetadata(rec->metadata);
	efree(rec);
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
	le_freeimage_tag = zend_register_list_destructors_ex(_php_freeimage_tag_dtor, NULL, le_freeimage_name_tag, module_number);
	le_freeimage_metadata = zend_register_list_destructors_ex(_php_freeimage_metadata_dtor, NULL, le_freeimage_name_metadata, module_number);
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
	REGISTER_FREEIMAGE_CONSTANT(FIF_HDR);
	REGISTER_FREEIMAGE_CONSTANT(FIF_FAXG3);
	REGISTER_FREEIMAGE_CONSTANT(FIF_SGI);
	REGISTER_FREEIMAGE_CONSTANT(FIF_EXR);
	REGISTER_FREEIMAGE_CONSTANT(FIF_J2K);
	REGISTER_FREEIMAGE_CONSTANT(FIF_JP2);

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
	REGISTER_FREEIMAGE_CONSTANT(FIT_RGB16);			// 48-bit RGB image			: 3 x 16-bit
	REGISTER_FREEIMAGE_CONSTANT(FIT_RGBA16);		// 64-bit RGBA image		: 4 x 16-bit
	REGISTER_FREEIMAGE_CONSTANT(FIT_RGBF);			// 96-bit RGB float image	: 3 x 32-bit IEEE floating point
	REGISTER_FREEIMAGE_CONSTANT(FIT_RGBAF);			// 128-bit RGBA float image	: 4 x 32-bit IEEE floating point

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
	REGISTER_FREEIMAGE_CONSTANT(FID_BAYER16x16);	// Bayer ordered dispersed dot dithering (order 4 dithering matrix)

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
	REGISTER_FREEIMAGE_CONSTANT(EXR_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(EXR_FLOAT);
	REGISTER_FREEIMAGE_CONSTANT(EXR_NONE);
	REGISTER_FREEIMAGE_CONSTANT(EXR_ZIP);
	REGISTER_FREEIMAGE_CONSTANT(EXR_PIZ);
	REGISTER_FREEIMAGE_CONSTANT(EXR_PXR24);
	REGISTER_FREEIMAGE_CONSTANT(EXR_B44);
	REGISTER_FREEIMAGE_CONSTANT(EXR_LC);
	REGISTER_FREEIMAGE_CONSTANT(FAXG3_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(GIF_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(GIF_LOAD256);
	REGISTER_FREEIMAGE_CONSTANT(GIF_PLAYBACK);
	REGISTER_FREEIMAGE_CONSTANT(HDR_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(ICO_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(ICO_MAKEALPHA);		// convert to 32bpp and create an alpha channel from the AND-mask when loading
	REGISTER_FREEIMAGE_CONSTANT(IFF_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(J2K_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(JP2_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_FAST);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_ACCURATE);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_CMYK);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_QUALITYSUPERB);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_QUALITYGOOD);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_QUALITYNORMAL);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_QUALITYAVERAGE);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_QUALITYBAD);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_PROGRESSIVE);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_SUBSAMPLING_411);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_SUBSAMPLING_420);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_SUBSAMPLING_422);
	REGISTER_FREEIMAGE_CONSTANT(JPEG_SUBSAMPLING_444);
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
	REGISTER_FREEIMAGE_CONSTANT(PNG_Z_BEST_SPEED);
	REGISTER_FREEIMAGE_CONSTANT(PNG_Z_DEFAULT_COMPRESSION);
	REGISTER_FREEIMAGE_CONSTANT(PNG_Z_NO_COMPRESSION);
	REGISTER_FREEIMAGE_CONSTANT(PNG_INTERLACED);
	REGISTER_FREEIMAGE_CONSTANT(PNM_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(PNM_SAVE_RAW);		// If set the writer saves in RAW format (i.e. P4, P5 or P6)
	REGISTER_FREEIMAGE_CONSTANT(PNM_SAVE_ASCII);	// If set the writer saves in ASCII format (i.e. P1, P2 or P3)
	REGISTER_FREEIMAGE_CONSTANT(PSD_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(RAS_DEFAULT);
	REGISTER_FREEIMAGE_CONSTANT(SGI_DEFAULT);
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

	/* Image metadata model constants */
	REGISTER_FREEIMAGE_CONSTANT(FIMD_NODATA);
	REGISTER_FREEIMAGE_CONSTANT(FIMD_COMMENTS);
	REGISTER_FREEIMAGE_CONSTANT(FIMD_EXIF_MAIN);
	REGISTER_FREEIMAGE_CONSTANT(FIMD_EXIF_EXIF);
	REGISTER_FREEIMAGE_CONSTANT(FIMD_EXIF_GPS);
	REGISTER_FREEIMAGE_CONSTANT(FIMD_EXIF_MAKERNOTE);
	REGISTER_FREEIMAGE_CONSTANT(FIMD_EXIF_INTEROP);
	REGISTER_FREEIMAGE_CONSTANT(FIMD_IPTC);
	REGISTER_FREEIMAGE_CONSTANT(FIMD_XMP);
	REGISTER_FREEIMAGE_CONSTANT(FIMD_GEOTIFF);
	REGISTER_FREEIMAGE_CONSTANT(FIMD_ANIMATION);
	REGISTER_FREEIMAGE_CONSTANT(FIMD_CUSTOM);

	/* Tag data type constants */
	REGISTER_FREEIMAGE_CONSTANT(FIDT_NOTYPE);
	REGISTER_FREEIMAGE_CONSTANT(FIDT_BYTE);
	REGISTER_FREEIMAGE_CONSTANT(FIDT_ASCII);
	REGISTER_FREEIMAGE_CONSTANT(FIDT_SHORT);
	REGISTER_FREEIMAGE_CONSTANT(FIDT_LONG);
	REGISTER_FREEIMAGE_CONSTANT(FIDT_RATIONAL);
	REGISTER_FREEIMAGE_CONSTANT(FIDT_SBYTE);
	REGISTER_FREEIMAGE_CONSTANT(FIDT_UNDEFINED);
	REGISTER_FREEIMAGE_CONSTANT(FIDT_SSHORT);
	REGISTER_FREEIMAGE_CONSTANT(FIDT_SLONG);
	REGISTER_FREEIMAGE_CONSTANT(FIDT_SRATIONAL);
	REGISTER_FREEIMAGE_CONSTANT(FIDT_FLOAT);
	REGISTER_FREEIMAGE_CONSTANT(FIDT_DOUBLE);
	REGISTER_FREEIMAGE_CONSTANT(FIDT_IFD);
	REGISTER_FREEIMAGE_CONSTANT(FIDT_PALETTE);

	

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

/* {{{ proto long freeimage_rgbquad(long red, long green, long blue)
   Creates a RGBQUAD value. */
PHP_FUNCTION(freeimage_rgbquad)
{
	int argc = ZEND_NUM_ARGS();
	long red, green, blue;

	if (zend_parse_parameters(argc TSRMLS_CC, "lll", &red, &green, &blue) == FAILURE) { 
		return;
	}

	RETURN_LONG(((red & 0xff)<<0) | ((green & 0xff)<<8) | ((blue & 0xff)<<16));
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
			// load from the handle
			bitmap = FreeImage_LoadFromHandle(fif, &io, (fi_handle)stream_buff, flags);
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Doesn't support reading this image type");
			goto error_close;
		}
	}
	if (bitmap != NULL) {
		BITMAP_REC *rec = (BITMAP_REC *)emalloc(sizeof(BITMAP_REC));
		rec->bitmap  = bitmap;
		rec->z_multi = NULL;
		ZEND_REGISTER_RESOURCE(return_value, rec, le_freeimage_bitmap);
		php_stream_close(stream);
		return;
	}

	php_error_docref(NULL TSRMLS_CC, E_WARNING, "'%s' is not a valid file", filename);
error_close:
	php_stream_close(stream);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool freeimage_save(resource bitmap, string filename [, int flags [, int fif]])
   Saves a previously loaded FIBITMAP to a file. */
PHP_FUNCTION(freeimage_save)
{
	char *filename = NULL;
	int argc = ZEND_NUM_ARGS();
	
	int filename_len;
	long flags = 0;
	long new_fif = FIF_UNKNOWN;
	zval *z_bitmap = NULL;
	BITMAP_REC *rec = NULL;
	FREE_IMAGE_FORMAT fif;
	unsigned bpp;

	if (zend_parse_parameters(argc TSRMLS_CC, "rs|ll", &z_bitmap, &filename, &filename_len, &flags, &new_fif) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);

	if (!filename || filename == "" || php_check_open_basedir(filename TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid filename '%s'", filename);
		RETURN_FALSE;
	}
	if (rec) {
		// determine output format
		fif = (FREE_IMAGE_FORMAT)new_fif;
		if (fif == FIF_UNKNOWN) {
			// try to guess the file format from the file extension
			fif = FreeImage_GetFIFFromFilename(filename);
		}
		// check that the plugin has writing capabilities ...
		if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsWriting(fif)) {
			bpp = FreeImage_GetBPP(rec->bitmap);
			if (fif == FIF_JPEG && (bpp != 24 && bpp != 8)) {
				// Convert JPEG to 24 bits before saving
				rec->bitmap = FreeImage_ConvertTo24Bits(rec->bitmap);
			} else if (fif == FIF_GIF && bpp != 8) {
				// Convert GIF to 8 bits before saving
				rec->bitmap = FreeImage_ConvertTo8Bits(rec->bitmap);
			}
			if (FreeImage_Save(fif, rec->bitmap, filename, flags)) {
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
	BITMAP_REC *rec = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	zend_list_delete(Z_RESVAL_P(z_bitmap));
}
/* }}} */

/* {{{ proto resource freeimage_clone(resource bitmap)
   Makes an exact reproduction of an existing bitmap. */
PHP_FUNCTION(freeimage_clone)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_bitmap = NULL;
	BITMAP_REC *rec = NULL;
	FIBITMAP *clone_bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	clone_bitmap = FreeImage_Clone(rec->bitmap);
	if (clone_bitmap != NULL) {
		BITMAP_REC* rec = (BITMAP_REC*)emalloc(sizeof(BITMAP_REC));
		rec->bitmap  = clone_bitmap;
		rec->z_multi = NULL;
		ZEND_REGISTER_RESOURCE(return_value, rec, le_freeimage_bitmap);
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
	BITMAP_REC *rec = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	imagetype = FreeImage_GetImageType(rec->bitmap);
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
	BITMAP_REC *rec = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	colors = FreeImage_GetColorsUsed(rec->bitmap);
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
	BITMAP_REC *rec = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	bpp = FreeImage_GetBPP(rec->bitmap);
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
	BITMAP_REC *rec = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	width = FreeImage_GetWidth(rec->bitmap);
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
	BITMAP_REC *rec = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	height = FreeImage_GetHeight(rec->bitmap);
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
	BITMAP_REC *rec = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	line = FreeImage_GetLine(rec->bitmap);
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
	BITMAP_REC *rec = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	pitch = FreeImage_GetPitch(rec->bitmap);
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
	BITMAP_REC *rec = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	size = FreeImage_GetDIBSize(rec->bitmap);
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
	BITMAP_REC *rec = NULL;
	FIBITMAP *bitmap = NULL, *rotate_bitmap = NULL;
	unsigned bpp;

	if (zend_parse_parameters(argc TSRMLS_CC, "rd", &z_bitmap, &angle) == FAILURE) 
		return;
	angle = (angle <0 || angle > 360) ? 0 : angle;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	bitmap = rec->bitmap;
	bpp = FreeImage_GetBPP(bitmap);
	// Convert to 24 bits if necessary
	if (bpp != 8 && bpp != 24 && bpp != 32) {
		bitmap = FreeImage_ConvertTo24Bits(bitmap);
	}
	rotate_bitmap = FreeImage_RotateClassic(bitmap, angle);
	if (bitmap != rec->bitmap) {
		FreeImage_Unload(bitmap);
	}

	if (rotate_bitmap != NULL) {
		BITMAP_REC* rec = (BITMAP_REC*)emalloc(sizeof(BITMAP_REC));
		rec->bitmap  = rotate_bitmap;
		rec->z_multi = NULL;
		ZEND_REGISTER_RESOURCE(return_value, rec, le_freeimage_bitmap);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_RotateClassic failed");
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
	BITMAP_REC *rec = NULL;
	FIBITMAP *bitmap = NULL, *rotate_bitmap = NULL;
	unsigned bpp;

	if (zend_parse_parameters(argc TSRMLS_CC, "rdddddb", &z_bitmap, &angle, &x_shift, &y_shift, &x_origin, &y_origin, &use_mask) == FAILURE) 
		return;
	angle		= (angle < 0 || angle > 360) ? 0 : angle;
	x_shift		= (x_shift < 0) ? 0 : x_shift;
	y_shift		= (y_shift < 0) ? 0 : y_shift;
	x_origin	= (x_origin < 0) ? 0 : x_origin;
	y_origin	= (y_origin < 0) ? 0 : y_origin;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	bitmap = rec->bitmap;
	bpp = FreeImage_GetBPP(bitmap);
	// Convert to 24 bits if necessary
	if (bpp != 8 && bpp != 24 && bpp != 32) {
		bitmap = FreeImage_ConvertTo24Bits(bitmap);
	}
	rotate_bitmap = FreeImage_RotateEx(bitmap, angle, x_shift, y_shift, x_origin, y_origin, use_mask);
	if (bitmap != rec->bitmap) {
		FreeImage_Unload(bitmap);
	}

	if (rotate_bitmap != NULL) {
		BITMAP_REC *rec = (BITMAP_REC *)emalloc(sizeof(BITMAP_REC));
		rec->bitmap  = rotate_bitmap;
		rec->z_multi = NULL;
		ZEND_REGISTER_RESOURCE(return_value, rec, le_freeimage_bitmap);
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
	BITMAP_REC *rec = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	if (FreeImage_FlipHorizontal(rec->bitmap)) {
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
	BITMAP_REC *rec = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	if (FreeImage_FlipVertical(rec->bitmap)) {
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
	BITMAP_REC* rec = NULL;
	FIBITMAP *bitmap = NULL, *rescale_bitmap = NULL;
	unsigned bpp;

	if (zend_parse_parameters(argc TSRMLS_CC, "rlll", &z_bitmap, &dst_width, &dst_height, &filter) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	bitmap = rec->bitmap;
	bpp = FreeImage_GetBPP(bitmap);
	// Convert to 24 bits if necessary
	if (bpp != 8 && bpp != 24 && bpp != 32) {
		bitmap = FreeImage_ConvertTo24Bits(bitmap);
	}
	rescale_bitmap = FreeImage_Rescale(bitmap, dst_width, dst_height, (FREE_IMAGE_FILTER)filter);
	if (bitmap != rec->bitmap) {
		FreeImage_Unload(bitmap);
	}

	if (rescale_bitmap != NULL) {
		BITMAP_REC *rec = (BITMAP_REC *)emalloc(sizeof(BITMAP_REC));
		rec->bitmap  = rescale_bitmap;
		rec->z_multi = NULL;
		ZEND_REGISTER_RESOURCE(return_value, rec, le_freeimage_bitmap);
	} else {
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
	BITMAP_REC *rec = NULL;
	FIBITMAP *bitmap = NULL;
	unsigned bpp;

	if (zend_parse_parameters(argc TSRMLS_CC, "rd", &z_bitmap, &gamma) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	bitmap = rec->bitmap;
	bpp = FreeImage_GetBPP(bitmap);
	// Convert to 24 bits if necessary
	if (bpp != 8 && bpp != 24 && bpp != 32) {
		bitmap = FreeImage_ConvertTo24Bits(bitmap);
	    FreeImage_Unload(rec->bitmap);
	    rec->bitmap = bitmap;
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
	BITMAP_REC *rec = NULL;
	FIBITMAP *bitmap = NULL;
	unsigned bpp;

	if (zend_parse_parameters(argc TSRMLS_CC, "rd", &z_bitmap, &percentage) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	bitmap = rec->bitmap;
	bpp = FreeImage_GetBPP(bitmap);
	// Convert to 24 bits if necessary
	if (bpp != 8 && bpp != 24 && bpp != 32) {
		bitmap = FreeImage_ConvertTo24Bits(bitmap);
	    FreeImage_Unload(rec->bitmap);
	    rec->bitmap = bitmap;
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
	BITMAP_REC *rec = NULL;
	FIBITMAP *bitmap = NULL;
	unsigned bpp;

	if (zend_parse_parameters(argc TSRMLS_CC, "rd", &z_bitmap, &percentage) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	bitmap = rec->bitmap;
	bpp = FreeImage_GetBPP(bitmap);
	// Convert to 24 bits if necessary
	if (bpp != 8 && bpp != 24 && bpp != 32) {
		bitmap = FreeImage_ConvertTo24Bits(bitmap);
	    FreeImage_Unload(rec->bitmap);
	    rec->bitmap = bitmap;
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
	BITMAP_REC *rec = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	if (FreeImage_Invert(rec->bitmap)) {
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
	BITMAP_REC *rec = NULL;
	FIBITMAP *copy_bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "rllll", &z_bitmap, &left, &top, &right, &bottom) == FAILURE) 
		return;
	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	copy_bitmap = FreeImage_Copy(rec->bitmap, left, top, right, bottom);
	if (copy_bitmap != NULL) {
		BITMAP_REC *rec = (BITMAP_REC *)emalloc(sizeof(BITMAP_REC));
		rec->bitmap  = copy_bitmap;
		rec->z_multi = NULL;
		ZEND_REGISTER_RESOURCE(return_value, rec, le_freeimage_bitmap);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_Copy failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool freeimage_paste(resource dst, resource src, int left, int top, int alpha)
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
	BITMAP_REC *dst_rec = NULL, *src_rec = NULL;
	FIBITMAP *dst_bitmap = NULL, *src_bitmap = NULL;
	unsigned bpp_dst, bpp_src;
    BOOL result;

	if (zend_parse_parameters(argc TSRMLS_CC, "rrlll", &z_dst, &z_src, &left, &top, &alpha) == FAILURE) 
		return;
	if (alpha <0 || alpha > 256) {
		alpha = 255;
	}

	ZEND_FETCH_RESOURCE(dst_rec, BITMAP_REC *, &z_dst, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	ZEND_FETCH_RESOURCE(src_rec, BITMAP_REC *, &z_src, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	
	dst_bitmap = dst_rec->bitmap;
	src_bitmap = src_rec->bitmap;
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
				case 32:
				default:
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
	result = FreeImage_Paste(dst_bitmap, src_bitmap, left, top, alpha);
	if (dst_bitmap != dst_rec->bitmap)
	{
		FreeImage_Unload(dst_rec->bitmap);
		dst_rec->bitmap = dst_bitmap;
	}
	if (src_bitmap != src_rec->bitmap) {
		FreeImage_Unload(src_bitmap);
	}
	RETURN_BOOL(result);
}
/* }}} */

/* {{{ proto resource freeimage_composite(resource fg [, bool usefilebkg [, long bkcolor [, resource bg]]])
   This function composite a foreground image against a single background color or 
   against a background image. */
PHP_FUNCTION(freeimage_composite)
{
	int argc = ZEND_NUM_ARGS();
	zend_bool usefilebkg = FALSE;
	zval *z_fg = NULL;
	long bkcolor = 0;
	zval *z_bg = NULL;
	BITMAP_REC *fg_rec = NULL, *bg_rec = NULL;
	FIBITMAP *fg_bitmap = NULL, *bg_bitmap = NULL, *new_bitmap = NULL;
	unsigned bpp_fg, bpp_bg;
	RGBQUAD bkrgb;
	unsigned fg_width, fg_height;

	if (zend_parse_parameters(argc TSRMLS_CC, "r|blr", &z_fg, &usefilebkg, &bkcolor, &z_bg) == FAILURE) 
		return;
	ZEND_FETCH_RESOURCE(fg_rec, BITMAP_REC *, &z_fg, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	fg_bitmap = fg_rec->bitmap;
	bpp_fg = FreeImage_GetBPP(fg_bitmap);
	if (bpp_fg != 8 && bpp_fg != 32) {
		fg_bitmap = FreeImage_ConvertTo32Bits(fg_bitmap);
		bpp_fg = 32;
	}
	new_bitmap = (FIBITMAP *)emalloc(sizeof(FIBITMAP));
	switch(argc) {
		case 1:
			// use a checkerboard background
			new_bitmap = FreeImage_Composite(fg_bitmap, FALSE, NULL, NULL);
			break;
		case 2:
			// use the image file background if there is one
			new_bitmap = FreeImage_Composite(fg_bitmap, usefilebkg, NULL, NULL);
			break;
		case 3:
			// use a user specified background colour
			_php_freeimage_long_to_rgbquad(bkcolor, &bkrgb);
			new_bitmap = FreeImage_Composite(fg_bitmap, FALSE, &bkrgb, NULL);
			break;
		case 4:
			// use bg as the background image
			// bg MUST BE a 24-bit image with the same width and height as fg
			ZEND_FETCH_RESOURCE(bg_rec, BITMAP_REC *, &z_bg, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
			bg_bitmap = bg_rec->bitmap;
			bpp_bg = FreeImage_GetBPP(bg_bitmap);
			if (bpp_bg != 24) {
				bg_bitmap = FreeImage_ConvertTo24Bits(bg_bitmap);
			}
			fg_width = FreeImage_GetWidth(fg_bitmap);
			fg_height = FreeImage_GetHeight(fg_bitmap);
			if ((FreeImage_GetWidth(bg_bitmap) != fg_width) ||
				(FreeImage_GetHeight(bg_bitmap) != fg_height))
			{
				FIBITMAP *scaled_bg = FreeImage_Rescale(bg_bitmap,
					fg_width, fg_height, FILTER_BOX);
				if (bg_bitmap != bg_rec->bitmap) {
					FreeImage_Unload(bg_bitmap);
				}	
				bg_bitmap = scaled_bg;
			}
			new_bitmap = FreeImage_Composite(fg_bitmap, FALSE, NULL, bg_bitmap);
			if (bg_bitmap != bg_rec->bitmap) {
				FreeImage_Unload(bg_bitmap);
			}
			break;
	}
	if (fg_bitmap != fg_rec->bitmap) {
		FreeImage_Unload(fg_bitmap);
	}

	if (new_bitmap != NULL) {
		BITMAP_REC* rec = (BITMAP_REC*)emalloc(sizeof(BITMAP_REC));
		rec->bitmap  = new_bitmap;
		rec->z_multi = NULL;
		ZEND_REGISTER_RESOURCE(return_value, rec, le_freeimage_bitmap);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_Composite failed");
		RETURN_FALSE;
	}
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
	BITMAP_REC *rec = NULL;
	FIBITMAP *conv_bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	conv_bitmap = FreeImage_ConvertTo8Bits(rec->bitmap);
	if (conv_bitmap != NULL) {
		BITMAP_REC *rec = (BITMAP_REC *)emalloc(sizeof(BITMAP_REC));
		rec->bitmap  = conv_bitmap;
		rec->z_multi = NULL;
		ZEND_REGISTER_RESOURCE(return_value, rec, le_freeimage_bitmap);
	} else {
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
	BITMAP_REC *rec = NULL;
	FIBITMAP *conv_bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	conv_bitmap = FreeImage_ConvertTo16Bits555(rec->bitmap);
	if (conv_bitmap != NULL) {
		BITMAP_REC *rec = (BITMAP_REC *)emalloc(sizeof(BITMAP_REC));
		rec->bitmap  = conv_bitmap;
		rec->z_multi = NULL;
		ZEND_REGISTER_RESOURCE(return_value, rec, le_freeimage_bitmap);
	} else {
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
	BITMAP_REC *rec = NULL;
	FIBITMAP *conv_bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	conv_bitmap = FreeImage_ConvertTo16Bits565(rec->bitmap);
	if (conv_bitmap != NULL) {
		BITMAP_REC *rec = (BITMAP_REC *)emalloc(sizeof(BITMAP_REC));
		rec->bitmap  = conv_bitmap;
		rec->z_multi = NULL;
		ZEND_REGISTER_RESOURCE(return_value, rec, le_freeimage_bitmap);
	} else {
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
	BITMAP_REC *rec = NULL;
	FIBITMAP *conv_bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	conv_bitmap = FreeImage_ConvertTo24Bits(rec->bitmap);
	if (conv_bitmap != NULL) {
		BITMAP_REC *rec = (BITMAP_REC *)emalloc(sizeof(BITMAP_REC));
		rec->bitmap  = conv_bitmap;
		rec->z_multi = NULL;
		ZEND_REGISTER_RESOURCE(return_value, rec, le_freeimage_bitmap);
	} else {
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
	BITMAP_REC *rec = NULL;
	FIBITMAP *conv_bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	conv_bitmap = FreeImage_ConvertTo32Bits(rec->bitmap);
	if (conv_bitmap != NULL) {
		BITMAP_REC *rec = (BITMAP_REC *)emalloc(sizeof(BITMAP_REC));
		rec->bitmap  = conv_bitmap;
		rec->z_multi = NULL;
		ZEND_REGISTER_RESOURCE(return_value, rec, le_freeimage_bitmap);
	} else {
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
	BITMAP_REC *rec = NULL;
	FIBITMAP *bitmap = NULL, *conv_bitmap = NULL;
	unsigned bpp;

	if (zend_parse_parameters(argc TSRMLS_CC, "rl", &z_bitmap, &quantize) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	bitmap = rec->bitmap;
	bpp = FreeImage_GetBPP(bitmap);
	if (bpp != 24) {
		// Convert to 24 bits if necessary
		bitmap = FreeImage_ConvertTo24Bits(bitmap);
	}
	conv_bitmap = FreeImage_ColorQuantize(bitmap, (FREE_IMAGE_QUANTIZE)quantize);
	if (bitmap != rec->bitmap) {
		FreeImage_Unload(bitmap);
	}

	if (conv_bitmap != NULL) {
		BITMAP_REC *rec = (BITMAP_REC *)emalloc(sizeof(BITMAP_REC));
		rec->bitmap  = conv_bitmap;
		rec->z_multi = NULL;
		ZEND_REGISTER_RESOURCE(return_value, rec, le_freeimage_bitmap);
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
	BITMAP_REC *rec = NULL;
	FIBITMAP *conv_bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "rl", &z_bitmap, &threshold) == FAILURE) 
		return;
	if (threshold < 0 || threshold > 255) {
		threshold = 128;
	}

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	conv_bitmap = FreeImage_Threshold(rec->bitmap, (BYTE)threshold);
	if (conv_bitmap != NULL) {
		BITMAP_REC *rec = (BITMAP_REC *)emalloc(sizeof(BITMAP_REC));
		rec->bitmap  = conv_bitmap;
		rec->z_multi = NULL;
		ZEND_REGISTER_RESOURCE(return_value, rec, le_freeimage_bitmap);
	} else {
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
	BITMAP_REC *rec = NULL;
	FIBITMAP *conv_bitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "rl", &z_bitmap, &algorithm) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	conv_bitmap = FreeImage_Dither(rec->bitmap, (FREE_IMAGE_DITHER)algorithm);
	if (conv_bitmap != NULL) {
		BITMAP_REC *rec = (BITMAP_REC *)emalloc(sizeof(BITMAP_REC));
		rec->bitmap  = conv_bitmap;
		rec->z_multi = NULL;
		ZEND_REGISTER_RESOURCE(return_value, rec, le_freeimage_bitmap);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_Dither failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto resource freeimage_openmultibitmap(int fif, string filename, bool create_new, bool read_only [, bool keep_cache_in_memory [, int flags]])
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
	long flags = 0;
	FIMULTIBITMAP *multibitmap;

	if (zend_parse_parameters(argc TSRMLS_CC, "lsbb|bl", &fif, &filename, &filename_len, &create_new, &read_only, &keep_cache_in_memory, &flags) == FAILURE) 
		return;
	multibitmap = FreeImage_OpenMultiBitmap((FREE_IMAGE_FORMAT)fif, filename, create_new, read_only, keep_cache_in_memory, flags);
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
	zend_list_delete(Z_RESVAL_P(z_multibitmap));
	RETURN_TRUE;
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
	BITMAP_REC *rec = NULL;
	if (zend_parse_parameters(argc TSRMLS_CC, "rr", &z_multibitmap, &z_bitmap) == FAILURE) 
		return;
	ZEND_FETCH_RESOURCE(multibitmap, FIMULTIBITMAP *, &z_multibitmap, -1, le_freeimage_name_multibitmap, le_freeimage_multibitmap);
	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	FreeImage_AppendPage(multibitmap, rec->bitmap);
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
	BITMAP_REC *rec = NULL;
	if (zend_parse_parameters(argc TSRMLS_CC, "rlr", &z_multibitmap, &page, &z_bitmap) == FAILURE) 
		return;
	ZEND_FETCH_RESOURCE(multibitmap, FIMULTIBITMAP *, &z_multibitmap, -1, le_freeimage_name_multibitmap, le_freeimage_multibitmap);
	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	FreeImage_InsertPage(multibitmap, page, rec->bitmap);
}
/* }}} */

/* {{{ proto void freeimage_deletepage(resource multibitmap, int page)
   Deletes the page on the given position.*/
PHP_FUNCTION(freeimage_deletepage)
{
	int argc = ZEND_NUM_ARGS();
	zval *z_multibitmap = NULL;
	long page = 0;
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
	if (!multibitmap)  return;
	lock_bitmap = FreeImage_LockPage(multibitmap, (int)page);
	if (lock_bitmap) {
		zend_list_addref(Z_RESVAL_P(z_multibitmap));
		BITMAP_REC *rec = (BITMAP_REC *)emalloc(sizeof(BITMAP_REC));
		rec->bitmap  = lock_bitmap;
		rec->z_multi = z_multibitmap;
		rec->changed = FALSE;
		ZEND_REGISTER_RESOURCE(return_value, rec, le_freeimage_bitmap);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_LockPage failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto void freeimage_unlockpage(resource multibitmap, resource bitmap [, bool changed])
   Unlocks a previously locked page and gives it back to the multi-page engine. 
   When the last parameter is TRUE, the page is marked changed and the new page data is applied 
   in the multi-page bitmap.*/
PHP_FUNCTION(freeimage_unlockpage)
{
	int argc = ZEND_NUM_ARGS();
	zval *z_multibitmap = NULL;
	zval *z_bitmap = NULL;
	zend_bool changed = FALSE;
	BITMAP_REC* rec = NULL;
	if (zend_parse_parameters(argc TSRMLS_CC, "rr|b", &z_multibitmap, &z_bitmap, &changed) == FAILURE) 
		return;
	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	if (z_multibitmap != rec->z_multi)  return;
	rec->changed = changed;
	zend_list_delete(Z_RESVAL_P(z_bitmap));
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

/* {{{ proto array freeimage_getlockedpagenumbers(resource multibitmap)
   Returns an array of page-numbers that are currently locked in memory.*/
PHP_FUNCTION(freeimage_getlockedpagenumbers)
{
	int argc = ZEND_NUM_ARGS();
	zval *z_multibitmap = NULL;
	int* pages;
	int count;
	FIMULTIBITMAP *multibitmap = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_multibitmap) == FAILURE) 
		return;
	ZEND_FETCH_RESOURCE(multibitmap, FIMULTIBITMAP *, &z_multibitmap, -1, le_freeimage_name_multibitmap, le_freeimage_multibitmap);
	if (!FreeImage_GetLockedPageNumbers(multibitmap, NULL, &count)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_GetLockedPageNumbers failed");
		RETURN_FALSE;
	}

	array_init(return_value);
	if (!count)
		return;

	pages = (int*)emalloc(count * sizeof(int));
	if (FreeImage_GetLockedPageNumbers(multibitmap, pages, &count)) {
		int n;
		for (n = 0;  n < count;  n++) {
			add_next_index_long(return_value, pages[n]);
		}
	}
	efree(pages);
}
/* }}} */

/* {{{ proto resource freeimage_createtag()
   Allocates a tag. */
PHP_FUNCTION(freeimage_createtag)
{
	FITAG *tag = NULL;
	if (ZEND_NUM_ARGS() != 0) {
		WRONG_PARAM_COUNT;
	}

	tag = FreeImage_CreateTag();
	if (tag != NULL) {
		TAG_REC *rec = (TAG_REC*)emalloc(sizeof(TAG_REC));
		rec->tag = tag;
		rec->allocated = 1;
		ZEND_REGISTER_RESOURCE(return_value, rec, le_freeimage_tag);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_CreateTag failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto void freeimage_deletetag(resource tag)
   Deletes a previously allocated tag. */
PHP_FUNCTION(freeimage_deletetag)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_rec = NULL;
	TAG_REC *rec = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_rec) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, TAG_REC *, &z_rec, -1, le_freeimage_name_tag, le_freeimage_tag);
	zend_list_delete(Z_RESVAL_P(z_rec));
}
/* }}} */

/* {{{ proto resource freeimage_clonetag(resource tag)
   Creates and returns a copy of a tag. */
PHP_FUNCTION(freeimage_clonetag)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_rec = NULL;
	TAG_REC *rec = NULL;
	FITAG *copy_tag = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_rec) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, TAG_REC *, &z_rec, -1, le_freeimage_name_tag, le_freeimage_tag);
	copy_tag = FreeImage_CloneTag(rec->tag);
	if (copy_tag != NULL) {
		TAG_REC *copy_rec = (TAG_REC*)emalloc(sizeof(TAG_REC));
		copy_rec->tag = copy_tag;
		copy_rec->allocated = 1;
		ZEND_REGISTER_RESOURCE(return_value, copy_rec, le_freeimage_tag);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "FreeImage_CloneTag failed");
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto string freeimage_gettagkey(resource tag)
   Returns the tag field name (unique inside a metadata model). */
PHP_FUNCTION(freeimage_gettagkey)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_rec = NULL;
	TAG_REC *rec = NULL;
	char *key = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_rec) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, TAG_REC *, &z_rec, -1, le_freeimage_name_tag, le_freeimage_tag);
	key = (char*)FreeImage_GetTagKey(rec->tag);
	RETURN_STRINGL(key, strlen(key), 1);
}
/* }}} */

/* {{{ proto mixed freeimage_gettagdescription(resource tag)
   Returns the tag description if available, returns NULL otherwise. */
PHP_FUNCTION(freeimage_gettagdescription)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_rec = NULL;
	TAG_REC *rec = NULL;
	char *desc = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_rec) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, TAG_REC *, &z_rec, -1, le_freeimage_name_tag, le_freeimage_tag);
	desc = (char*)FreeImage_GetTagDescription(rec->tag);
    if (desc) {
		RETURN_STRINGL(desc, strlen(desc), 1);
	} else {
		RETURN_NULL();
	}
}
/* }}} */

/* {{{ proto int freeimage_gettagid(resource tag)
   Returns the tag ID if available, returns 0 otherwise. */
PHP_FUNCTION(freeimage_gettagid)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_rec = NULL;
	TAG_REC *rec = NULL;
    unsigned id = 0;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_rec) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, TAG_REC *, &z_rec, -1, le_freeimage_name_tag, le_freeimage_tag);
	id = FreeImage_GetTagID(rec->tag);
    RETURN_LONG((long)id);
}
/* }}} */

/* {{{ proto int freeimage_gettagtype(resource tag)
   Returns the tag data type. */
PHP_FUNCTION(freeimage_gettagtype)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_rec = NULL;
	TAG_REC *rec = NULL;
    unsigned type = 0;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_rec) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, TAG_REC *, &z_rec, -1, le_freeimage_name_tag, le_freeimage_tag);
	type = FreeImage_GetTagType(rec->tag);
    RETURN_LONG((long)type);
}
/* }}} */

/* {{{ proto int freeimage_gettagcount(resource tag)
   Returns the number of components in the tag (in tag type units). */
PHP_FUNCTION(freeimage_gettagcount)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_rec = NULL;
	TAG_REC *rec = NULL;
    unsigned count = 0;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_rec) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, TAG_REC *, &z_rec, -1, le_freeimage_name_tag, le_freeimage_tag);
	count = FreeImage_GetTagCount(rec->tag);
    RETURN_LONG((long)count);
}
/* }}} */

/* {{{ proto string freeimage_gettagvalue(resource tag)
   Returns the length of the tag value in bytes. */
PHP_FUNCTION(freeimage_gettagvalue)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_rec = NULL;
	TAG_REC *rec = NULL;
    char* value = NULL;
    unsigned length = 0;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_rec) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, TAG_REC *, &z_rec, -1, le_freeimage_name_tag, le_freeimage_tag);
	value = (char*)FreeImage_GetTagValue(rec->tag);
    length = FreeImage_GetTagLength(rec->tag);
	RETURN_STRINGL(value, length, 1);
}
/* }}} */

/* {{{ proto bool freeimage_settagkey(resource tag, string key)
   Set the tag field name(always required, must be unique inside a metadata model).
   Returns TRUE on success, FALSE on failure. */
PHP_FUNCTION(freeimage_settagkey)
{
	int argc = ZEND_NUM_ARGS();
	zval *z_rec = NULL;
	TAG_REC *rec = NULL;
	char *key = NULL;
	int key_len;

	if (zend_parse_parameters(argc TSRMLS_CC, "rs", &z_rec, &key, &key_len) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, TAG_REC *, &z_rec, -1, le_freeimage_name_tag, le_freeimage_tag);
	if (FreeImage_SetTagKey(rec->tag, key)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool freeimage_settagdescription(resource tag, string description)
   Set the (ususally optional) tag description.
   Returns TRUE on success, FALSE on failure. */
PHP_FUNCTION(freeimage_settagdescription)
{
	int argc = ZEND_NUM_ARGS();
	zval *z_rec = NULL;
	TAG_REC *rec = NULL;
	char *desc = NULL;
	int desc_len;

	if (zend_parse_parameters(argc TSRMLS_CC, "rs", &z_rec, &desc, &desc_len) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, TAG_REC *, &z_rec, -1, le_freeimage_name_tag, le_freeimage_tag);
	if (FreeImage_SetTagDescription(rec->tag, desc)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool freeimage_settagid(resource tag, int id)
   Set the (ususally optional) tag ID.
   Returns TRUE on success, FALSE on failure. */
PHP_FUNCTION(freeimage_settagid)
{
	int argc = ZEND_NUM_ARGS();
	zval *z_rec = NULL;
	TAG_REC *rec = NULL;
    long id = 0;

	if (zend_parse_parameters(argc TSRMLS_CC, "rl", &z_rec, &id) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, TAG_REC *, &z_rec, -1, le_freeimage_name_tag, le_freeimage_tag);
	if (FreeImage_SetTagID(rec->tag, id)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool freeimage_settagtype(resource tag, int type)
   Set the tag data type (always required).
   Returns TRUE on success, FALSE on failure. */
PHP_FUNCTION(freeimage_settagtype)
{
	int argc = ZEND_NUM_ARGS();
	zval *z_rec = NULL;
	TAG_REC *rec = NULL;
    long type = 0;

	if (zend_parse_parameters(argc TSRMLS_CC, "rl", &z_rec, &type) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, TAG_REC *, &z_rec, -1, le_freeimage_name_tag, le_freeimage_tag);
	if (FreeImage_SetTagType(rec->tag, type)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool freeimage_settagcount(resource tag, int count)
   Set the number of data in the tag(always required, expressed in tag type unit).
   Returns TRUE on success, FALSE on failure. */
PHP_FUNCTION(freeimage_settagcount)
{
	int argc = ZEND_NUM_ARGS();
	zval *z_rec = NULL;
	TAG_REC *rec = NULL;
    long count = 0;

	if (zend_parse_parameters(argc TSRMLS_CC, "rl", &z_rec, &count) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, TAG_REC *, &z_rec, -1, le_freeimage_name_tag, le_freeimage_tag);
	if (FreeImage_SetTagCount(rec->tag, count)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto bool freeimage_settagvalue(resource tag, string value)
   Set the length of the tag value, in bytes (always required).
   Returns TRUE on success, FALSE on failure. */
PHP_FUNCTION(freeimage_settagvalue)
{
	int argc = ZEND_NUM_ARGS();
	zval *z_rec = NULL;
	TAG_REC *rec = NULL;
    char *value = NULL;
    int value_len;

	if (zend_parse_parameters(argc TSRMLS_CC, "rs", &z_rec, &value, &value_len) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, TAG_REC *, &z_rec, -1, le_freeimage_name_tag, le_freeimage_tag);
	if (FreeImage_SetTagLength(rec->tag, value_len)) {
		if (FreeImage_SetTagValue(rec->tag, value)) {
			RETURN_TRUE;
		}
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto mixed freeimage_findfirstmetadata(int model, resource bitmap)
   Begins searching for the first instance of a tag that matches the required metadata model.
   Returns a search handle identifying the group of tags on success, NULL on failure. */
PHP_FUNCTION(freeimage_findfirstmetadata)
{
	int argc = ZEND_NUM_ARGS();
	
    long model = 0;
	zval *z_bitmap = NULL;
	BITMAP_REC *rec = NULL;
	FIMETADATA *metadata = NULL;
    FITAG *tag = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "lr", &model, &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	metadata = FreeImage_FindFirstMetadata(model, rec->bitmap, &tag);
	if (metadata != NULL) {
		METADATA_REC *rec = (METADATA_REC*)emalloc(sizeof(METADATA_REC));
		rec->metadata = metadata;
		rec->tag = tag;
		ZEND_REGISTER_RESOURCE(return_value, rec, le_freeimage_bitmap);
	} else {
		RETURN_NULL();
	}
}
/* }}} */

/* {{{ proto bool freeimage_findnextmetadata(resource metadata)
   Find the next tag, if any, that matches the metadata model argument in a previous call
   to FreeImage_FindFirstMetadata.
   Returns TRUE on success, FALSE on failure. */
PHP_FUNCTION(freeimage_findnextmetadata)
{
	int argc = ZEND_NUM_ARGS();
	
    long model = 0;
	zval *z_rec = NULL;
    METADATA_REC *rec = NULL;
    FITAG *tag = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_rec) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, METADATA_REC *, &z_rec, -1, le_freeimage_name_metadata, le_freeimage_metadata);
	if (FreeImage_FindNextMetadata(rec->metadata, &tag)) {
		rec->tag = tag;
		RETURN_TRUE;
	} else {
		rec->tag = NULL;
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto void freeimage_findclosemetadata(resource metadata)
   Closes the specified metadata search handle. */
PHP_FUNCTION(freeimage_findclosemetadata)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_rec = NULL;
	METADATA_REC *rec = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_rec) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, METADATA_REC *, &z_rec, -1, le_freeimage_name_metadata, le_freeimage_metadata);
	zend_list_delete(Z_RESVAL_P(z_rec));
}
/* }}} */

/* {{{ proto mixed freeimage_findtag(resource metadata)
   Returns current tag from a metadata search handle if available, NULL otherwise. */
PHP_FUNCTION(freeimage_findtag)
{
	int argc = ZEND_NUM_ARGS();
	
	zval *z_rec = NULL;
	METADATA_REC *rec = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "r", &z_rec) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, METADATA_REC *, &z_rec, -1, le_freeimage_name_metadata, le_freeimage_metadata);
	if (rec->tag) {
		TAG_REC *tag_rec = (TAG_REC*)emalloc(sizeof(TAG_REC));
		tag_rec->tag = rec->tag;
		tag_rec->allocated = 0;
		ZEND_REGISTER_RESOURCE(return_value, tag_rec, le_freeimage_tag);
	} else {
		RETURN_NULL();
	}
}
/* }}} */

/* {{{ proto mixed freeimage_getmetadata(int model, resource bitmap, string key)
   Retrieve a tag that matches the required metadata model and key inside a bitmap.
   Returns a tag handle on success, NULL on failure. */
PHP_FUNCTION(freeimage_getmetadata)
{
	int argc = ZEND_NUM_ARGS();
	
    long model = 0;
	zval *z_bitmap = NULL;
	char *key = NULL;
	int key_len;
	BITMAP_REC *rec = NULL;
    FITAG *tag = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "lrs", &model, &z_bitmap, &key, &key_len) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	if (FreeImage_GetMetadata(model, rec->bitmap, key, &tag)) {
		TAG_REC *rec = (TAG_REC*)emalloc(sizeof(TAG_REC));
		rec->tag = tag;
		rec->allocated = 0;
		ZEND_REGISTER_RESOURCE(return_value, rec, le_freeimage_tag);
	} else {
		RETURN_NULL();
	}
}
/* }}} */

/* {{{ proto bool freeimage_setmetadata(int model, resource bitmap, string key, resource tag)
   Attach a tag to a bitmap using the specified metadata model and key.
   If tag is NULL then the metadata is deleted.
   If both key and tag are NULL then the metadata model is deleted.
   Returns TRUE on success, FALSE on failure. */
PHP_FUNCTION(freeimage_setmetadata)
{
	int argc = ZEND_NUM_ARGS();
	
    long model = 0;
	zval *z_bitmap = NULL;
	char *key = NULL;
	int key_len;
	zval *z_rec = NULL;
	BITMAP_REC *rec = NULL;
	FITAG* tag = NULL;

	if (zend_parse_parameters(argc TSRMLS_CC, "lrs!r!", &model, &z_bitmap, &key, &key_len, &z_rec) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	if (z_rec) {
		TAG_REC *rec = NULL;
		ZEND_FETCH_RESOURCE(rec, TAG_REC *, &z_rec, -1, le_freeimage_name_tag, le_freeimage_tag);
		tag = rec->tag;
	}
	if (FreeImage_SetMetadata(model, rec->bitmap, key, tag)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto int freeimage_getmetadatacount(int model, resource bitmap)
   Retrieve the number of tags attached to the bitmap matching the specified metadata model. */
PHP_FUNCTION(freeimage_getmetadatacount)
{
	int argc = ZEND_NUM_ARGS();
	
    long model = 0;
	zval *z_bitmap = NULL;
	BITMAP_REC *rec = NULL;
    unsigned count = 0;

	if (zend_parse_parameters(argc TSRMLS_CC, "lr", &model, &z_bitmap) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, BITMAP_REC *, &z_bitmap, -1, le_freeimage_name_bitmap, le_freeimage_bitmap);
	count = FreeImage_GetMetadataCount(model, rec->bitmap);
	RETURN_LONG((long)count);
}
/* }}} */

/* {{{ proto string freeimage_tagtostring(int model, resource tag [, string make])
   Converts a tag to a string that represents the interpreted tag value.
   Make is the camera model. */
PHP_FUNCTION(freeimage_tagtostring)
{
	int argc = ZEND_NUM_ARGS();
	
    long model = 0;
	zval *z_rec = NULL;
	char *make = NULL;
	int make_len;
	TAG_REC *rec = NULL;
	char *tag_str;

	if (zend_parse_parameters(argc TSRMLS_CC, "lr|s", &model, &z_rec, &make, &make_len) == FAILURE) 
		return;

	ZEND_FETCH_RESOURCE(rec, TAG_REC *, &z_rec, -1, le_freeimage_name_tag, le_freeimage_tag);
	tag_str = (char *)FreeImage_TagToString(model, rec->tag, make);
	RETURN_STRINGL(tag_str, strlen(tag_str), 1);
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
