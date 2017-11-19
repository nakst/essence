ColorBlue='\033[0;36m'
ColorNormal='\033[0m'

Optimise=""
OptimiseKernel="-DDEBUG_BUILD"

BuildFlags="-ffreestanding -Wall -Wextra -fno-exceptions -mcmodel=large -fno-rtti -g -DARCH_64 -DARCH_X86_64 -DARCH_X86_COMMON -std=c++11 -Wno-frame-address"
LinkFlags="-T linker_userland64.ld -ffreestanding -nostdlib -lgcc -g -z max-page-size=0x1000 -Lbin/os -lapi"
KernelLinkFlags="-ffreestanding -nostdlib -lgcc -g -z max-page-size=0x1000"

echo -e "-> Building ${ColorBlue}API${ColorNormal}..."
nasm -felf64 api/api.s -o bin/os/api1.o -Fdwarf
x86_64-elf-g++ -c api/api.cpp -o bin/os/api2.o $BuildFlags -Wno-unused-function $Optimise
x86_64-elf-ar -rcs bin/os/libapi.a bin/os/api1.o bin/os/api2.o

echo -e "-> Building ${ColorBlue}executive${ColorNormal}..."
x86_64-elf-g++ -c executive/main.cpp -o bin/os/executive.o $BuildFlags  $Optimise
x86_64-elf-gcc -o bin/os/executive bin/os/executive.o $LinkFlags

echo -e "-> Building ${ColorBlue}calculator${ColorNormal}..."
x86_64-elf-g++ -c calculator/main.cpp -o bin/os/calculator.o $BuildFlags  $Optimise
x86_64-elf-gcc -o bin/os/calculator bin/os/calculator.o $LinkFlags

echo -e "-> Building ${ColorBlue}kernel${ColorNormal}..."
nasm -felf64 kernel/x86_64.s -o bin/os/kernel_x86_64.o -Fdwarf
x86_64-elf-g++ -c kernel/main.cpp -o bin/os/kernel.o -mno-red-zone $BuildFlags $OptimiseKernel
x86_64-elf-gcc -T linker64.ld -o bin/os/kernel bin/os/kernel_x86_64.o bin/os/kernel.o -mno-red-zone $KernelLinkFlags

echo "-> Saving debug symbols..."
cp bin/os/kernel bin/os/kernel_symbols
cp bin/os/executive bin/os/executive_symbols
cp bin/os/calculator bin/os/calculator_symbols

echo "-> Removing temporary files..."
x86_64-elf-strip --strip-all bin/os/kernel
x86_64-elf-strip --strip-all bin/os/executive
x86_64-elf-strip --strip-all bin/os/test
x86_64-elf-strip --strip-all bin/os/calculator
rm bin/os/*.o
