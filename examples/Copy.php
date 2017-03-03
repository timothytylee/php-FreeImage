<?php
$dib = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
$newdib = FreeImage_Copy($dib, 0, 0, 100, 100);
FreeImage_Save($newdib, dirname(__FILE__) . "/php_copy.jpg", 0);
FreeImage_Free($newdib);
FreeImage_Free($dib);

// OO API
/*
$fi = new FreeImage();
$dib = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
$newdib = $fi->Copy($dib, 0, 0, 100, 100);
$fi->Save($newdib, dirname(__FILE__) . "/php_copy.jpg", 0);
$fi->Free($newdib);
$fi->Free($dib);
*/
?>
