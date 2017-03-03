<?php
$dib = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.jpg", 0);
$new_dib = FreeImage_Rotate($dib, 30);
FreeImage_Save($new_dib, dirname(__FILE__) . "/php_rotate.jpg", 0);
$new_dib = FreeImage_RotateEx($dib, 30, 0, 0, 0, 0, true);
FreeImage_Save($new_dib, dirname(__FILE__) . "/php_rotateex.jpg", 0);
FreeImage_Free($dib);
FreeImage_Free($new_dib);

// OO API
/*
$fi = new FreeImage();
$dib = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.jpg", 0);
$new_dib = $fi->Rotate($dib, 30);
$fi->Save($new_dib, dirname(__FILE__) . "/php_rotate.jpg", 0);
$new_dib = $fi->RotateEx($dib, 30, 0, 0, 0, 0, true);
$fi->Save($new_dib, dirname(__FILE__) . "/php_rotateex.jpg", 0);
$fi->Free($dib);
$fi->Free($new_dib);
*/
?>
