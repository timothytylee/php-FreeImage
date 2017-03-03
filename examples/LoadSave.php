<?php
$dib = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.psd", 0);
echo FreeImage_GetLastError();
FreeImage_Save($dib, dirname(__FILE__) . "/php_save.bmp", BMP_DEFAULT);
echo FreeImage_GetLastError();
FreeImage_Free($dib);

// OO API
/*
$fi = new FreeImage();
$dib = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
$fi->Save($dib, dirname(__FILE__) . "/php_save.bmp", 0);
$fi->Free($dib);
*/
?>
