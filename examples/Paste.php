<?php
$dst = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
$src = FreeImage_Load(dirname(__FILE__) . "/php_copy.jpg", 0);
FreeImage_Paste($dst, $src, 20, 20, 100);
FreeImage_Save($dst, dirname(__FILE__) . "/php_paste.jpg", 0);
FreeImage_Free($dst);
FreeImage_Free($src);

// OO API
/*
$fi = new FreeImage();
$dst = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
$src = $fi->Load(dirname(__FILE__) . "/php_copy.jpg", 0);
$fi->Paste($dst, $src, 20, 20, 100);
$fi->Save($dst, dirname(__FILE__) . "/php_paste.jpg", 0);
$fi->Free($dst);
$fi->Free($src);
*/
?>
