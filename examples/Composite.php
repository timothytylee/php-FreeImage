<?php
$bg = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.png", 0);
$fg = FreeImage_Rotate($bg, 45);
$new_dib = FreeImage_Composite($fg);
FreeImage_Save($new_dib, dirname(__FILE__) . "/php_composite_checker.jpg", 0);
FreeImage_Free($new_dib);
$new_dib = FreeImage_Composite($fg, false, FreeImage_RgbQuad(0, 255, 0));
FreeImage_Save($new_dib, dirname(__FILE__) . "/php_composite_bgcolor.jpg", 0);
FreeImage_Free($new_dib);
$new_dib = FreeImage_Composite($fg, false, 0, $bg);
FreeImage_Save($new_dib, dirname(__FILE__) . "/php_composite_bgimage.jpg", 0);
FreeImage_Free($new_dib);
FreeImage_Free($bg);
FreeImage_Free($fg);

// OO API
/*
$fi = new FreeImage();
$bg = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.png", 0);
$fg = $fi->Rotate($bg, 45);
$new_dib = $fi->Composite($fg);
$fi->Save($new_dib, dirname(__FILE__) . "/php_composite_checker.jpg", 0);
$fi->Free($new_dib);
$new_dib = $fi->Composite($fg, false, $fi->RgbQuad(0, 255, 0));
$fi->Save($new_dib, dirname(__FILE__) . "/php_composite_bgcolor.jpg", 0);
$fi->Free($new_dib);
$new_dib = $fi->Composite($fg, false, 0, $bg);
$fi->Save($new_dib, dirname(__FILE__) . "/php_composite_bgimage.jpg", 0);
$fi->Free($new_dib);
$fi->Free($bg);
$fi->Free($fg);
*/
?>
