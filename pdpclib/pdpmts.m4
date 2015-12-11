
$cre mtsmacs.mac

$copy *source* mtsmacs.mac(1)
PDPTOP    100
PDPPRLG   200
PDPEPIL   300
PDPMAIN   400
00000000
$ENDFILE
$copy *source* mtsmacs.mac(100)
undivert(pdptop.mac)$ENDFILE
$copy *source* mtsmacs.mac(200)
undivert(pdpprlg.mac)$ENDFILE
$copy *source* mtsmacs.mac(300)
undivert(pdpepil.mac)$ENDFILE
$copy *source* mtsmacs.mac(400)
undivert(pdpmain.mac)$ENDFILE

$cre mtsstart.asm
$cre mtsstart.r
$cre mtsstart.l
$cre mtsstart.err

$copy *source* to mtsstart.asm
undivert(mtsstart.asm)$ENDFILE

$run *asmg scards=mtsstart.asm sprint=mtsstart.l 2=mtsmacs.mac

# spunch=mtsstart.r sprint=mtsstart.l
# sercom=mtsstart.err par=test

$li mtsstart.l

$r mtsstart.r
