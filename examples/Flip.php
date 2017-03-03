<?php
$dib = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
if (FreeImage_FlipHorizontal($dib)) {
	echo 'Flip Horizontal';
}
FreeImage_Save($dib, dirname(__FILE__) . "/php_fliphor.jpg", 0);
FreeImage_Free($dib);
$dib = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
if (FreeImage_FlipVertical($dib)) {
	echo 'Flip Vertical';
}
FreeImage_Save($dib, dirname(__FILE__) . "/php_flipver.jpg", 0);
FreeImage_Free($dib);

// OO API
/*
$fi = new FreeImage();
$dib = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
if ($fi->FlipHorizontal($dib)) {
	echo 'Flip Horizontal';
}
$fi->Save($dib, dirname(__FILE__) . "/php_fliphor.jpg", 0);
$fi->Free($dib);
$dib = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
if ($fi->FlipVertical($dib)) {
	echo 'Flip Vertical';
}
$fi->Save($dib, dirname(__FILE__) . "/php_flipver.jpg", 0);
$fi->Free($dib);
*/
?>
