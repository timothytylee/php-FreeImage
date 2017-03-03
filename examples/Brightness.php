<?php
$dib = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
if (FreeImage_Brightness($dib, 50)) {
	echo 'Increase Brightness';
}
FreeImage_Save($dib, dirname(__FILE__) . "/php_brightness_increase.jpg", 0);
FreeImage_Free($dib);
$dib = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
if (FreeImage_Brightness($dib, -50)) {
	echo 'Decrease Brightness';
}
FreeImage_Save($dib, dirname(__FILE__) . "/php_brightness_decrease.jpg", 0);
FreeImage_Free($dib);

// OO API
/*
$fi = new FreeImage();
$dib = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
if ($fi->Brightness($dib, 50)) {
	echo 'Increase Brightness';
}
$fi->Save($dib, dirname(__FILE__) . "/php_brightness_increase.jpg", 0);
$fi->Free($dib);
$dib = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
if ($fi->Brightness($dib, -50)) {
	echo 'Decrease Brightness';
}
$fi->Save($dib, dirname(__FILE__) . "/php_brightness_decrease.jpg", 0);
$fi->Free($dib);
*/
?>
