
$cre mtsstart.asm
$cre mtsstart.r
$cre mtsstart.l
$cre mtsstart.err

$copy *source* to mtsstart.asm
undivert(mtsstart.asm)$ENDFILE

$run *asmg scards=mtsstart.asm spunch=mtsstart.r sprint=mtsstart.l

# sercom=mtsstart.err par=test

$li mtsstart.l

$r mtsstart.r
