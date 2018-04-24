ColorBlue='\033[0;36m'
ColorNormal='\033[0m'

export Optimise=$1
OptimiseKernel=$2

export BuildFlags="-ffreestanding -Wall -Wextra -Wno-missing-field-initializers -fno-exceptions -mcmodel=large -fno-rtti -g -DARCH_64 -DARCH_X86_64 -DARCH_X86_COMMON -std=c++11 -Wno-frame-address -Ifreetype"
export LinkFlags="-T util/linker_userland64.ld -ffreestanding -nostdlib -lgcc -g -z max-page-size=0x1000 -Lbin/OS -lapi -lfreetype -Lfreetype"
KernelLinkFlags="-ffreestanding -nostdlib -lgcc -g -z max-page-size=0x1000"

echo -e "-> Building ${ColorBlue}API${ColorNormal}..."
./manifest_parser api/empty.manifest bin/OS/standard.manifest.h
nasm -felf64 api/api.s -o bin/OS/api1.o -Fdwarf
nasm -felf64 api/crti.s -o bin/OS/crti.o -Fdwarf
nasm -felf64 api/crtn.s -o bin/OS/crtn.o -Fdwarf
x86_64-elf-g++ -c api/api.cpp -o bin/OS/api2.o $BuildFlags -Wno-unused-function $Optimise
x86_64-elf-ar -rcs bin/OS/libapi.a bin/OS/api1.o bin/OS/api2.o 
x86_64-elf-g++ -c api/glue.cpp -o bin/OS/glue.o $BuildFlags -Wno-unused-function $Optimise
x86_64-elf-ar -rcs bin/OS/libglue.a bin/OS/glue.o 

echo -e "-> Building ${ColorBlue}desktop${ColorNormal}..."
./manifest_parser desktop/desktop.manifest bin/OS/desktop.manifest.h
echo -e "-> Building ${ColorBlue}test program${ColorNormal}..."
./manifest_parser api/test.manifest bin/OS/test.manifest.h
echo -e "-> Building ${ColorBlue}calculator${ColorNormal}..."
./manifest_parser calculator/calculator.manifest bin/OS/calculator.manifest.h
echo -e "-> Building ${ColorBlue}file manager${ColorNormal}..."
./manifest_parser file_manager/file_manager.manifest bin/OS/file_manager.manifest.h
echo -e "-> Building ${ColorBlue}image viewer${ColorNormal}..."
./manifest_parser image_viewer/image_viewer.manifest bin/OS/image_viewer.manifest.h

echo -e "-> Building ${ColorBlue}kernel${ColorNormal}..."
nasm -felf64 kernel/x86_64.s -o bin/OS/kernel_x86_64.o -Fdwarf
x86_64-elf-g++ -c kernel/main.cpp -o bin/OS/kernel.o -mno-red-zone $BuildFlags $OptimiseKernel
x86_64-elf-gcc -T util/linker64.ld -o bin/OS/Kernel.esx bin/OS/kernel_x86_64.o bin/OS/kernel.o -mno-red-zone $KernelLinkFlags
cp bin/OS/Kernel.esx bin/OS/Kernel.esx_symbols
x86_64-elf-strip --strip-all bin/OS/Kernel.esx

echo "-> Removing temporary files..."
rm bin/OS/*.o
