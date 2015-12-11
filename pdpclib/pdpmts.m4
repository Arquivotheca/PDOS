
$cre mtsstart.asm
$cre mtsstart.r
$cre mtsstart.l
$cre mtsstart.err

$copy *source* file(1)
PDPTOP    10
PDPPRLG   20
PDPEPIL   30
00000000
$ENDFILE
$copy *source* file(10)
undivert(pdptop.mac)$ENDFILE
$copy *source* file(20)
undivert(pdpprlg.mac)$ENDFILE
$copy *source* file(30)
undivert(pdpepil.mac)$ENDFILE

$copy *source* to mtsstart.asm
undivert(mtsstart.asm)$ENDFILE

$run *asmg scards=mtsstart.asm spunch=mtsstart.r sprint=mtsstart.l

# sercom=mtsstart.err par=test

$li mtsstart.l

$r mtsstart.r
