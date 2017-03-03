<?php
$dib = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
if (FreeImage_Gamma($dib, 0.5)) {
	echo 'Increase Gamma';
}
FreeImage_Save($dib, dirname(__FILE__) . "/php_gamma_increase.jpg", 0);
FreeImage_Free($dib);
$dib = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
if (FreeImage_Gamma($dib, 1.0)) {
	echo 'Decrease Gamma';
}
FreeImage_Save($dib, dirname(__FILE__) . "/php_gamma_decrease.jpg", 0);
FreeImage_Free($dib);

// OO API
/*
$fi = new FreeImage();
$dib = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
if ($fi->Gamma($dib, 50)) {
	echo 'Increase Gamma';
}
$fi->Save($dib, dirname(__FILE__) . "/php_gamma_increase.jpg", 0);
$fi->Free($dib);
$dib = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
if ($fi->Gamma($dib, -50)) {
	echo 'Decrease Gamma';
}
$fi->Save($dib, dirname(__FILE__) . "/php_gamma_decrease.jpg", 0);
$fi->Free($dib);
*/
?>
