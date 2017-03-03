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

  $Id: php_freeimage.h,v 1.4 2004/07/22 13:32:54 wenlong Exp $ 
*/

#ifndef PHP_FREEIMAGE_H
#define PHP_FREEIMAGE_H

extern zend_module_entry freeimage_module_entry;
#define phpext_freeimage_ptr &freeimage_module_entry

#ifdef PHP_WIN32
#define PHP_FREEIMAGE_API __declspec(dllexport)
#else
#define PHP_FREEIMAGE_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define PECL_FREEIMAGE_VERSION		"0.1-dev"
#define PECL_FREEIMAGE_AUTHOR		"Wenlong Wu <ezdevelop@hotmail.com>"

PHP_MINIT_FUNCTION(freeimage);
PHP_MSHUTDOWN_FUNCTION(freeimage);
PHP_RINIT_FUNCTION(freeimage);
PHP_RSHUTDOWN_FUNCTION(freeimage);
PHP_MINFO_FUNCTION(freeimage);

PHP_FUNCTION(freeimage_constructor);
/* Bitmap management functions */
PHP_FUNCTION(freeimage_load);
PHP_FUNCTION(freeimage_save);
PHP_FUNCTION(freeimage_unload);
PHP_FUNCTION(freeimage_clone);
/* General functions */
PHP_FUNCTION(freeimage_getversion);
PHP_FUNCTION(freeimage_getcopyrightmessage);
PHP_FUNCTION(freeimage_getlasterror);
/* Bitmap infomation functions */
PHP_FUNCTION(freeimage_getimagetype);
PHP_FUNCTION(freeimage_getcolorsused);
PHP_FUNCTION(freeimage_getbpp);
PHP_FUNCTION(freeimage_getwidth);
PHP_FUNCTION(freeimage_getheight);
PHP_FUNCTION(freeimage_getline);
PHP_FUNCTION(freeimage_getpitch);
PHP_FUNCTION(freeimage_getdibsize);
/* Filetype functions */
PHP_FUNCTION(freeimage_getfiletype);
/* Plugin functions */
PHP_FUNCTION(freeimage_getfifcount);
PHP_FUNCTION(freeimage_setpluginenabled);
PHP_FUNCTION(freeimage_ispluginenabled);
PHP_FUNCTION(freeimage_getfiffromformat);
PHP_FUNCTION(freeimage_getfiffrommime);
PHP_FUNCTION(freeimage_getformatfromfif);
PHP_FUNCTION(freeimage_getfifextensionlist);
PHP_FUNCTION(freeimage_getfifdescription);
PHP_FUNCTION(freeimage_getfifregexpr);
PHP_FUNCTION(freeimage_getfiffromfilename);
PHP_FUNCTION(freeimage_fifsupportsreading);
PHP_FUNCTION(freeimage_fifsupportswriting);
/* Tookit & Conversion functions */
PHP_FUNCTION(freeimage_rotate);
PHP_FUNCTION(freeimage_rotateex);
PHP_FUNCTION(freeimage_fliphorizontal);
PHP_FUNCTION(freeimage_flipvertical);
PHP_FUNCTION(freeimage_rescale);
PHP_FUNCTION(freeimage_gamma);
PHP_FUNCTION(freeimage_brightness);
PHP_FUNCTION(freeimage_contrast);
PHP_FUNCTION(freeimage_invert);
PHP_FUNCTION(freeimage_copy);
PHP_FUNCTION(freeimage_paste);
PHP_FUNCTION(freeimage_composite);
PHP_FUNCTION(freeimage_convertto8bits);
PHP_FUNCTION(freeimage_convertto16bits555);
PHP_FUNCTION(freeimage_convertto16bits565);
PHP_FUNCTION(freeimage_convertto24bits);
PHP_FUNCTION(freeimage_convertto32bits);
PHP_FUNCTION(freeimage_colorquantize);
PHP_FUNCTION(freeimage_threshold);
PHP_FUNCTION(freeimage_dither);
/* Multipage functions */
PHP_FUNCTION(freeimage_openmultibitmap);
PHP_FUNCTION(freeimage_closemultibitmap);
PHP_FUNCTION(freeimage_getpagecount);
PHP_FUNCTION(freeimage_appendpage);
PHP_FUNCTION(freeimage_insertpage);
PHP_FUNCTION(freeimage_deletepage);
PHP_FUNCTION(freeimage_lockpage);
PHP_FUNCTION(freeimage_unlockpage);
PHP_FUNCTION(freeimage_movepage);
PHP_FUNCTION(freeimage_getlockedpagenumbers);


ZEND_BEGIN_MODULE_GLOBALS(freeimage)
	char *freeimage_lasterror;
ZEND_END_MODULE_GLOBALS(freeimage)

/* In every utility function you add that needs to use variables 
   in php_freeimage_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as FREEIMAGE_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define FREEIMAGE_G(v) TSRMG(freeimage_globals_id, zend_freeimage_globals *, v)
#else
#define FREEIMAGE_G(v) (freeimage_globals.v)
#endif

#endif	/* PHP_FREEIMAGE_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
