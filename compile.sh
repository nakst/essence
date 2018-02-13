ColorBlue='\033[0;36m'
ColorNormal='\033[0m'

Optimise=$1
OptimiseKernel=$2

BuildFlags="-ffreestanding -Wall -Wextra -fno-exceptions -mcmodel=large -fno-rtti -g -DARCH_64 -DARCH_X86_64 -DARCH_X86_COMMON -std=c++11 -Wno-frame-address -Ifreetype"
LinkFlags="-T util/linker_userland64.ld -ffreestanding -nostdlib -lgcc -g -z max-page-size=0x1000 -Lbin/os -lapi -lfreetype -Lfreetype"
KernelLinkFlags="-ffreestanding -nostdlib -lgcc -g -z max-page-size=0x1000"

echo -e "-> Building ${ColorBlue}API${ColorNormal}..."
nasm -felf64 api/api.s -o bin/os/api1.o -Fdwarf
x86_64-elf-g++ -c api/api.cpp -o bin/os/api2.o $BuildFlags -Wno-unused-function $Optimise
x86_64-elf-ar -rcs bin/os/libapi.a bin/os/api1.o bin/os/api2.o

echo -e "-> Building ${ColorBlue}desktop${ColorNormal}..."
x86_64-elf-g++ -c desktop/main.cpp -o bin/os/desktop.o $BuildFlags  $Optimise
x86_64-elf-gcc -o bin/os/desktop bin/os/desktop.o $LinkFlags
cp bin/os/desktop bin/os/desktop_symbols
x86_64-elf-strip --strip-all bin/os/desktop

echo -e "-> Building ${ColorBlue}test program${ColorNormal}..."
x86_64-elf-g++ -c api/test.cpp -o bin/os/test.o $BuildFlags  $Optimise 
x86_64-elf-gcc -o bin/os/test bin/os/test.o $LinkFlags 
cp bin/os/test bin/os/test_symbols
x86_64-elf-strip --strip-all bin/os/test

echo -e "-> Building ${ColorBlue}kernel${ColorNormal}..."
nasm -felf64 kernel/x86_64.s -o bin/os/kernel_x86_64.o -Fdwarf
x86_64-elf-g++ -c kernel/main.cpp -o bin/os/kernel.o -mno-red-zone $BuildFlags $OptimiseKernel
x86_64-elf-gcc -T util/linker64.ld -o bin/os/kernel bin/os/kernel_x86_64.o bin/os/kernel.o -mno-red-zone $KernelLinkFlags
cp bin/os/kernel bin/os/kernel_symbols
x86_64-elf-strip --strip-all bin/os/kernel

echo "-> Removing temporary files..."
rm bin/os/*.o
