<?php
$dst = FreeImage_OpenMultiBitmap(FIF_GIF,
	dirname(__FILE__) . "/php_metadata.gif", true, false);
$org = FreeImage_Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
$frame_time = 1000;
$levels = array(0.5, 1.0);
$tag = FreeImage_CreateTag();
FreeImage_SetTagKey($tag, "FrameTime");
FreeImage_SetTagType($tag, FIDT_LONG);
FreeImage_SetTagCount($tag, 1);
FreeImage_SetTagValue($tag, pack("l", $frame_time));
$p = 1;
foreach ($levels as $g)
{
	$src = FreeImage_ConvertTo24Bits($org);
	FreeImage_Gamma($src, $g);
	$src = FreeImage_ColorQuantize($src, FIQ_WUQUANT);
	FreeImage_SetMetadata(FIMD_ANIMATION, $src, null, null);
	FreeImage_SetMetadata(FIMD_ANIMATION, $src, FreeImage_GetTagKey($tag), $tag);
	echo "Page $p duration is $frame_time ms<br>";
	FreeImage_AppendPage($dst, $src);
	FreeImage_Free($src);
	$p++;
}
FreeImage_DeleteTag($tag);
FreeImage_Free($org);
FreeImage_CloseMultiBitmap($dst);

// OO API
/*
$fi = new FreeImage();
$dst = $fi->OpenMultiBitmap(FIF_GIF,
	dirname(__FILE__) . "/php_metadata.gif", true, false);
$org = $fi->Load(dirname(__FILE__) . "/orig_images/php_orig.tif", 0);
$frame_time = 1000;
$levels = array(0.5, 1.0);
$tag = $fi->CreateTag();
$fi->SetTagKey($tag, "FrameTime");
$fi->SetTagType($tag, FIDT_LONG);
$fi->SetTagCount($tag, 1);
$fi->SetTagValue($tag, pack("l", $frame_time));
$p = 1;
foreach ($levels as $g)
{
	$src = $fi->ConvertTo24Bits($org);
	$fi->Gamma($src, $g);
	$src = $fi->ColorQuantize($src, FIQ_WUQUANT);
	$fi->SetMetadata(FIMD_ANIMATION, $src, null, null);
	$fi->SetMetadata(FIMD_ANIMATION, $src, $fi->GetTagKey($tag), $tag);
	echo "Page $p duration is $frame_time ms<br>";
	$fi->AppendPage($dst, $src);
	$fi->Free($src);
	$p++;
}
$fi->DeleteTag($tag);
$fi->Free($org);
$fi->CloseMultiBitmap($dst);
*/
?>
