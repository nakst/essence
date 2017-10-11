./build.sh
qemu-system-x86_64 -drive file=drive,format=raw,media=disk,index=0 -m 2048 -s -S -smp cores=1 > /dev/null 2>&1
#qemu-system-x86_64 -drive file=drive,format=raw,media=disk,index=0 -m 2048 -s -S -smp cores=4 > /dev/null 2>&1
