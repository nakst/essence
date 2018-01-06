./build.sh
# qemu-system-x86_64 -drive file=drive,format=raw,media=disk,index=0 -m 2048 -s -S -smp cores=1 > /dev/null 2>&1
# qemu-system-x86_64 -drive file=drive,format=raw,media=disk,index=0 -m 2048 -s -S -smp cores=4 > /dev/null 2>&1
qemu-system-x86_64 -drive file=drive,if=none,id=mydisk,format=raw,media=disk,index=0 -device ich9-ahci,id=ahci -device ide-drive,drive=mydisk,bus=ahci.0 -m 64 -s -S -smp cores=1 > log.txt 2>&1
