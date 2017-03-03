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

  $Id: freeimage_io.c,v 1.1 2004/07/22 13:32:54 wenlong Exp $ 
*/

#include <stdio.h>
#include <stdlib.h>

#include "php.h"
#include "php_freeimage_io.h"

fi_handle freeimageio_memptr;

/* {{{ FreeImageIO_MemoryReadProc
 */
unsigned DLL_CALLCONV
FreeImageIO_MemoryReadProc(void *buffer, unsigned size, unsigned count, fi_handle handle)
{
	BYTE *tmp = (BYTE *)buffer;
	unsigned c;
	for (c = 0; c < count; c++) {
		memcpy(tmp, freeimageio_memptr, size);

		freeimageio_memptr = (BYTE *)freeimageio_memptr + size;

		tmp += size;
	}

	return count;
}
/* }}} */

/* {{{ FreeImageIO_MemoryWriteProc
 */
unsigned DLL_CALLCONV
FreeImageIO_MemoryWriteProc(void *buffer, unsigned size, unsigned count, fi_handle handle)
{
	// there's not much use for saving the bitmap into memory now, is there?
	// do nothing, because there's not much use for saving the bitmap into memory.
	return size;
}
/* }}} */

/* {{{ FreeImageIO_MemorySeekProc
 */
int DLL_CALLCONV
FreeImageIO_MemorySeekProc(fi_handle handle, long offset, int origin) 
{
	assert(origin != SEEK_END);

	if (origin == SEEK_SET) {
		freeimageio_memptr = (BYTE *)handle + offset;
	} else {
		freeimageio_memptr = (BYTE *)freeimageio_memptr + offset;
	}

	return 0;
}
/* }}} */

/* {{{ FreeImageIO_MemoryTellProc
 */
long DLL_CALLCONV
FreeImageIO_MemoryTellProc(fi_handle handle)
{
	assert((int)handle > (int)freeimageio_memptr);

	return ((int)freeimageio_memptr - (int)handle);
}
/* }}} */

/* {{{ FreeImageIO_FileTellProc
 */
unsigned DLL_CALLCONV 
FreeImageIO_FileReadProc(void *buffer, unsigned size, unsigned count, fi_handle handle) {
	return fread(buffer, size, count, (FILE *)handle);
}
/* }}} */

/* {{{ FreeImageIO_FileTellProc
 */
unsigned DLL_CALLCONV 
FreeImageIO_FileWriteProc(void *buffer, unsigned size, unsigned count, fi_handle handle) {
	return fwrite(buffer, size, count, (FILE *)handle);
}
/* }}} */

/* {{{ FreeImageIO_FileTellProc
 */
int DLL_CALLCONV
FreeImageIO_FileSeekProc(fi_handle handle, long offset, int origin) {
	return fseek((FILE *)handle, offset, origin);
}
/* }}} */

/* {{{ FreeImageIO_FileTellProc
 */
long DLL_CALLCONV
FreeImageIO_FileTellProc(fi_handle handle) {
	return ftell((FILE *)handle);
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
