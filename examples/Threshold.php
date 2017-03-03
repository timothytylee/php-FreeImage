<?php
$dib = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
$newdib = FreeImage_Threshold($dib, 1);
$newdib2 = FreeImage_Threshold($dib, 255);
FreeImage_Save($newdib, dirname(__FILE__) . "/php_threshold-1.jpg", 0);
FreeImage_Save($newdib2, dirname(__FILE__) . "/php_threshold-255.jpg", 0);
FreeImage_Free($dib);
FreeImage_Free($newdib);
FreeImage_Free($newdib2);

// OO API
/*
$fi = new FreeImage();
$dib = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
$newdib = $fi->Threshold($dib, 1);
$newdib2 = $fi->Threshold($dib, 255);
$fi->Save($newdib, dirname(__FILE__) . "/php_threshold-1.jpg", 0);
$fi->Save($newdib2, dirname(__FILE__) . "/php_threshold-255.jpg", 0);
$fi->Free($dib);
$fi->Free($newdib);
$fi->Free($newdib2);
*/
?>
