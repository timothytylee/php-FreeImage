<?php
$dib = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
$newdib = FreeImage_ColorQuantize($dib, FIQ_WUQUANT);
FreeImage_Save($newdib, dirname(__FILE__) . "/php_colorquantize_wuquant.jpg", 0);
$newdib2 = FreeImage_ColorQuantize($dib, FIQ_NNQUANT);
FreeImage_Save($newdib2, dirname(__FILE__) . "/php_colorquantize_nnquant.jpg", 0);
FreeImage_Free($dib);
FreeImage_Free($newdib);
FreeImage_Free($newdib2);

// OO API
/*
$fi = new FreeImage();
$dib = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
$newdib = $fi->ColorQuantize($dib, FIQ_WUQUANT);
$fi->Save($newdib, dirname(__FILE__) . "/php_colorquantize_wuquant.jpg", 0);
$newdib2 = $fi->ColorQuantize($dib, FIQ_NNQUANT);
$fi->Save($newdib2, dirname(__FILE__) . "/php_colorquantize_nnquant.jpg", 0);
$fi->Free($dib);
$fi->Free($newdib);
$fi->Free($newdib2);
*/
?>
