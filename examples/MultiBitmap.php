<?php
$dst = FreeImage_OpenMultiBitmap(FIF_GIF,
	dirname(__FILE__) . "/php_multibitmap.gif", true, false);
$org = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
$levels = array(0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 0.9, 0.8, 0.7, 0.6);
$p = 1;
foreach ($levels as $g)
{
	$src = FreeImage_ConvertTo24Bits($org);
	FreeImage_Gamma($src, $g);
	$src = FreeImage_ColorQuantize($src, FIQ_WUQUANT);
	FreeImage_AppendPage($dst, $src);
	echo "Appended page $p<br>";
	FreeImage_Free($src);
	$p++;
}
FreeImage_Free($org);
FreeImage_CloseMultiBitmap($dst);

// OO API
/*
$fi = new FreeImage();
$dst = $fi->OpenMultiBitmap(FIF_GIF,
	dirname(__FILE__) . "/php_multibitmap.gif", true, false);
$org = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
$levels = array(0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 0.9, 0.8, 0.7, 0.6);
$p = 1;
foreach ($levels as $g)
{
	$src = $fi->ConvertTo24Bits($org);
	$fi->Gamma($src, $g);
	$src = $fi->ColorQuantize($src, FIQ_WUQUANT);
	$fi->AppendPage($dst, $src);
	echo "Appended page $p<br>";
	$fi->Free($src);
	$p++;
}
$fi->Free($org);
$fi->CloseMultiBitmap($dst);
*/
?>
