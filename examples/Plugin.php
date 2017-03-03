<?php
echo FreeImage_GetLastError();
printf("FreeImage Version %s<br>%s<br><br>", FreeImage_GetVersion(), FreeImage_GetCopyrightMessage());
for ($i=0; $i<FreeImage_GetFIFCount(); $i++) {
	printf("<b>Bitmap type %d:</b> <u>%s</u> (%s)<br>", $i, FreeImage_GetFIFDescription($i), FreeImage_GetFIFExtensionList($i));
	echo str_repeat('&nbsp;', 17);
	if (FreeImage_FIFSupportsReading($i)) {
		echo 'Read: <font color="green"><b>Enabled</b></font>';
	} else {
		echo 'Read: <font color="red"><b>Disabled</b></font>';
	}
	if (FreeImage_FIFSupportsWriting($i)) {
		echo ' - Write: <font color="green"><b>Enabled</b></font><br>';
	} else {
		echo ' - Write: <font color="red"><b>Disabled</b></font><br>';
	}
}

// OO API
/*
$fi = new FreeImage();
printf("FreeImage Version %s<br>%s<br><br>", $fi->GetVersion(), $fi->GetCopyrightMessage());
for ($i=0; $i<$fi->GetFIFCount(); $i++) {
	printf("<b>Bitmap type %d:</b> <u>%s</u> (%s)<br>", $i, $fi->GetFIFDescription($i), $fi->GetFIFExtensionList($i));
	echo str_repeat('&nbsp;', 17);
	if ($fi->FIFSupportsReading($i)) {
		echo 'Read: <font color="green"><b>Enabled</b></font>';
	} else {
		echo 'Read: <font color="red"><b>Disabled</b></font>';
	}
	if ($fi->FIFSupportsWriting($i)) {
		echo ' - Write: <font color="green"><b>Enabled</b></font><br>';
	} else {
		echo ' - Write: <font color="red"><b>Disabled</b></font><br>';
	}
}
*/
?>
