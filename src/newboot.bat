sectread 81 63 boot.com
xychop pbootsec.com boot.001 0 10
xychop pbootsec.com boot.003 73 511
xychop boot.com boot.002 11 72
copy /b boot.001+boot.002+boot.003 boot2.com
sectwrit 81 63 boot2.com
