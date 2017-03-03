<?php
$dib = FreeImage_Load("http://www.php.net/images/logos/php_script.ico", 0);
FreeImage_Save($dib, dirname(__FILE__) . "/php_loadfromurl.ico", 0);
FreeImage_Free($dib);

// OO API
/*
$fi = new FreeImage();
$dib = $fi->Load("http://www.php.net/images/logos/php_script.ico", 0);
$fi->Save($dib, dirname(__FILE__) . "/php_loadfromurl.bmp", BMP_DEFAULT);
$fi->Free($dib);
*/
?>
