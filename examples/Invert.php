<?php
$dib = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
if (FreeImage_Invert($dib)) {
	echo 'Invert';
}
FreeImage_Save($dib, dirname(__FILE__) . "/php_invert.jpg", 0);
FreeImage_Free($dib);

// OO API
/*
$fi = new FreeImage();
$dib = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
if ($fi->Invert($dib)) {
	echo 'Increase Invert';
}
$fi->Save($dib, dirname(__FILE__) . "/php_invert.jpg", 0);
$fi->Free($dib);
*/
?>
