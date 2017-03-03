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

  $Id: php_freeimage_io.h,v 1.1 2004/07/22 13:32:54 wenlong Exp $ 
*/

#ifndef PHP_FREEIMAGE_IO_H
#define PHP_FREEIMAGE_IO_H

#include "FreeImage.h"

#define FREEIMAGEIO_FROM_MEMORY	1
#define FREEIMAGEIO_FROM_FILE	2

typedef struct {
	void *data;
	long size;
	long curpos;
} fi_handle_io;


extern fi_handle freeimageio_memptr;
extern unsigned DLL_CALLCONV FreeImageIO_MemoryReadProc(void *buffer, unsigned size, unsigned count, fi_handle handle);
extern unsigned DLL_CALLCONV FreeImageIO_MemoryWriteProc(void *buffer, unsigned size, unsigned count, fi_handle handle);
extern int DLL_CALLCONV FreeImageIO_MemorySeekProc(fi_handle handle, long offset, int origin);
extern long DLL_CALLCONV FreeImageIO_MemoryTellProc(fi_handle handle);

extern unsigned DLL_CALLCONV FreeImageIO_FileReadProc(void *buffer, unsigned size, unsigned count, fi_handle handle);
extern unsigned DLL_CALLCONV FreeImageIO_FileWriteProc(void *buffer, unsigned size, unsigned count, fi_handle handle);
extern int DLL_CALLCONV FreeImageIO_FileSeekProc(fi_handle handle, long offset, int origin);
extern long DLL_CALLCONV FreeImageIO_FileTellProc(fi_handle handle);

#endif	/* PHP_FREEIMAGE_IO_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */