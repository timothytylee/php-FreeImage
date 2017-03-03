<?php
$dib = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
if (FreeImage_Contrast($dib, 50)) {
	echo 'Increase Contrast';
}
FreeImage_Save($dib, dirname(__FILE__) . "/php_contrast_increase.jpg", 0);
FreeImage_Free($dib);
$dib = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
if (FreeImage_Contrast($dib, -50)) {
	echo 'Decrease Contrast';
}
FreeImage_Save($dib, dirname(__FILE__) . "/php_contrast_decrease.jpg", 0);
FreeImage_Free($dib);

// OO API
/*
$fi = new FreeImage();
$dib = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
if ($fi->Contrast($dib, 50)) {
	echo 'Increase Contrast';
}
$fi->Save($dib, dirname(__FILE__) . "/php_contrast_increase.jpg", 0);
$fi->Free($dib);
$dib = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
if ($fi->Contrast($dib, -50)) {
	echo 'Decrease Contrast';
}
$fi->Save($dib, dirname(__FILE__) . "/php_contrast_decrease.jpg", 0);
$fi->Free($dib);
*/
?>
