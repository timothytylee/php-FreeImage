<?php
$dib = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
$newdib = FreeImage_Dither($dib, FID_FS);
FreeImage_Save($newdib, dirname(__FILE__) . "/php_dither_fs.jpg", 0);
FreeImage_Free($newdib);
$newdib = FreeImage_Dither($dib, FID_BAYER4x4);
FreeImage_Save($newdib, dirname(__FILE__) . "/php_dither_bayer4x4.jpg", 0);
FreeImage_Free($newdib);
$newdib = FreeImage_Dither($dib, FID_BAYER8x8);
FreeImage_Save($newdib, dirname(__FILE__) . "/php_dither_bayer8x8.jpg", 0);
FreeImage_Free($newdib);
$newdib = FreeImage_Dither($dib, FID_CLUSTER6x6);
FreeImage_Save($newdib, dirname(__FILE__) . "/php_dither_cluster6x6.jpg", 0);
FreeImage_Free($newdib);
$newdib = FreeImage_Dither($dib, FID_CLUSTER8x8);
FreeImage_Save($newdib, dirname(__FILE__) . "/php_dither_cluster8x8.jpg", 0);
FreeImage_Free($newdib);
$newdib = FreeImage_Dither($dib, FID_CLUSTER16x16);
FreeImage_Save($newdib, dirname(__FILE__) . "/php_dither_cluster16x16.jpg", 0);
FreeImage_Free($newdib);
FreeImage_Free($dib);

// OO API
/*
$fi = new FreeImage();
$dib = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
$newdib = $fi->Dither($dib, FID_FS);
$fi->Save($newdib, dirname(__FILE__) . "/php_dither_fs.jpg", 0);
$fi->Free($newdib);
$newdib = $fi->Dither($dib, FID_BAYER4x4);
$fi->Save($newdib, dirname(__FILE__) . "/php_dither_bayer4x4.jpg", 0);
$fi->Free($newdib);
$newdib = $fi->Dither($dib, FID_BAYER8x8);
$fi->Save($newdib, dirname(__FILE__) . "/php_dither_bayer8x8.jpg", 0);
$fi->Free($newdib);
$newdib = $fi->Dither($dib, FID_CLUSTER6x6);
$fi->Save($newdib, dirname(__FILE__) . "/php_dither_cluster6x6.jpg", 0);
$fi->Free($newdib);
$newdib = $fi->Dither($dib, FID_CLUSTER8x8);
$fi->Save($newdib, dirname(__FILE__) . "/php_dither_cluster8x8.jpg", 0);
$fi->Free($newdib);
$newdib = $fi->Dither($dib, FID_CLUSTER16x16);
$fi->Save($newdib, dirname(__FILE__) . "/php_dither_cluster16x16.jpg", 0);
$fi->Free($newdib);
$fi->Free($dib);
*/
?>
