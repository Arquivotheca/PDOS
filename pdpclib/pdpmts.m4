
$dest mtsmacs.mac ok
$cre mtsmacs.mac

$copy *source* mtsmacs.mac(1)
PDPPRLG   100
PDPEPIL   200
PDPMAIN   300
PDPTOP    400
00000000
$endfile
$copy *source* mtsmacs.mac(100)
undivert(pdpprlg.mac)$endfile
$copy *source* mtsmacs.mac(200)
undivert(pdpepil.mac)$endfile
$copy *source* mtsmacs.mac(300)
undivert(pdpmain.mac)$endfile
$copy *source* mtsmacs.mac(400)
undivert(pdptop.mac)$endfile

li mtsmacs.mac *print*


$dest mtsstart.asm ok
$cre mtsstart.asm
$copy *source* to mtsstart.asm
undivert(mtsstart.asm)$endfile


*
* define a macro for repeated assemblies
*
>macro dobld n
>define srcf="{n}.asm"
>define runf="{n}.r"
>define listf="{n}.l"
>define errf="{n}.e"
>define rest="2=mtsmacs.mac par=test"

$dest {listf} ok
$cr {listf}

$dest {errf} ok
$cr {errf}

$dest {runf} ok
$cre {runf}

$run *asmg scards={srcf} spunch={runf} sprint={listf} sercom={errf} {rest}

li {listf} *print*
li {errf} *print*

>endmacro



dobld mtsstart

$r mtsstart.r

