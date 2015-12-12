
$dest mtsmacs.mac
$cre mtsmacs.mac

$copy *source* mtsmacs.mac(1)
PDPTOP    100
PDPPRLG   200
PDPEPIL   300
PDPMAIN   400
00000000
$endfile
$copy *source* mtsmacs.mac(100)
undivert(pdptop.mac)$endfile
$copy *source* mtsmacs.mac(200)
undivert(pdpprlg.mac)$endfile
$copy *source* mtsmacs.mac(300)
undivert(pdpepil.mac)$endfile
$copy *source* mtsmacs.mac(400)
undivert(pdpmain.mac)$endfile

li mtsmacs.mac *print*


$cre mtsstart.asm
$cre mtsstart.r
$cre mtsstart.l
$cre mtsstart.err

$copy *source* to mtsstart.asm
undivert(mtsstart.asm)$endfile

$dest dobld.m ok
$cre dobld.m
*
* need '>>' for copy to create single '>'
*
$copy *source* to dobld.m
>>macro dobld
>>define srcf="mtsstart.asm"
>>define runf="mtsstart.r"
>>define listf="mtsstart.l"
>>define errf="mtsstart.e"
>>define rest="2=mtsmacs.mac par=test"

$dest {runf} ok
$cre {runf}

>>* $em {rf} ok

$dest {listf} ok
$cr {listf}

$dest {errf} ok
$cr {errf}

$dest {runf} ok
$cre {runf}

$run *asmg scards={srcf} spunch={runf} sprint={listf} sercom={errf} {rest}

li {listf} *print*
li {errf} *print*

>>endmacro
$endfile

$sou dobld.m

dobld

$r mtsstart.r

