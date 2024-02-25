<?php
function convpad($in,$len=11) {
	$s=base_convert($in,10,8);
	$n=strlen($s);
	for($i=0;$i<$len-$n;$i++)
		$s="0".$s;
	return $s;
}
function tarhdr($name,$size,$mtime,$linkto=false,$mode="0000644",$uid="0000000",$gid="0000000") {
	$mode="0000755";
	$uid="0000000";
	$hdr=$name;
	for($i=strlen($hdr);$i<100;$i++)
		$hdr[$i]="\0";
	$hdr.=$mode."\0".$uid."\0".$gid."\0".convpad($size)."\0".convpad($mtime)."\0"."        ";
	for($i=strlen($hdr);$i<512;$i++)
		$hdr[$i]="\0";
	$cksum=0;
	for($i=0;$i<strlen($hdr);$i++)
		$cksum+=ord($hdr[$i]);
	$hdr=substr_replace($hdr,convpad($cksum,7)."\0",148,8);
	for($i=strlen($hdr);$i<512;$i++)
		$hdr.="\0";
	return $hdr;
}
function tarpad($nwritten) {
	$pad="";
	for($i=0;$i<512-($nwritten % 512);$i++)
		$pad.="\0";
	return($pad);
}
function patchelf($s) {
	$norig=ord($s[0x38]);
	$s[0x38]=chr(2); // e_phnum
	$norig-=ord($s[0x38]);
	$ph1=substr($s, 0x40, 0x38); 
	$ph3=substr($s, 0x40+0x38*1, 0x38); 
	$null=str_repeat(chr(0), 0x38);
	$ph=$ph1.$ph3;
	for($i=0; $i<$norig; $i++) $ph.=$null;
	$n=strlen($ph);
	for($i=0; $i<$n; ++$i) $s[0x40+$i]=$ph[$i];
	return($s);
}
$n=filesize("httpredir");
print tarhdr("e", $n, 0);
print patchelf(file_get_contents("httpredir"));
print tarpad($n);

