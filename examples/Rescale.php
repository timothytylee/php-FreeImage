<?php
$dib = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.tif", TIFF_DEFAULT);
$newdib = FreeImage_Rescale($dib, 160, 80, FILTER_BOX);
FreeImage_Save($newdib, dirname(__FILE__) . "/php_rescale.jpg", JPEG_DEFAULT);
FreeImage_Free($dib);
FreeImage_Free($newdib);

// OO API
/*
$fi = new FreeImage();
$dib = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.tif", TIFF_DEFAULT);
$newdib = $fi->Rescale($dib, 160, 80, FILTER_BOX);
$fi->Save($newdib, dirname(__FILE__) . "/php_rescale.jpg", JPEG_DEFAULT);
$fi->Free($dib);
$fi->Free($newdib);
*/
?>