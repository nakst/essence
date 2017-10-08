nasm -felf64 api/api.s -o bin/os/api1.o -Fdwarf
x86_64-elf-g++ -c api/api.cpp -o bin/os/api2.o -ffreestanding -Wall -Wextra -fno-exceptions -mcmodel=large -fno-rtti -g -DARCH_64 -DARCH_X86_64 -DARCH_X86_COMMON -std=c++11 -Wno-frame-address    -O3
x86_64-elf-ar -rcs bin/os/libapi.a bin/os/api1.o bin/os/api2.o

x86_64-elf-g++ -c shell/main.cpp -o bin/os/shell.o -ffreestanding -Wall -Wextra -fno-exceptions -mcmodel=large -fno-rtti -g -DOS_H_HELPER_FUNCTIONS -DARCH_64 -DARCH_X86_64 -DARCH_X86_COMMON -std=c++11 -Wno-frame-address  -O3
x86_64-elf-gcc -T linker_userland64.ld -o bin/os/shell -ffreestanding -nostdlib bin/os/shell.o -lgcc -g -z max-page-size=0x1000 -Lbin/os -lapi  

x86_64-elf-g++ -c test_program/main.cpp -o bin/os/test.o -ffreestanding -Wall -Wextra -fno-exceptions -mcmodel=large -fno-rtti -g -DOS_H_HELPER_FUNCTIONS -DARCH_64 -DARCH_X86_64 -DARCH_X86_COMMON -std=c++11 -Wno-frame-address  -O3
x86_64-elf-gcc -T linker_userland64.ld -o bin/os/test -ffreestanding -nostdlib bin/os/test.o -lgcc -g -z max-page-size=0x1000 -Lbin/os -lapi  

nasm -felf64 kernel/x86_64.s -o bin/os/kernel_x86_64.o -Fdwarf
x86_64-elf-g++ -c kernel/main.cpp -o bin/os/kernel.o -ffreestanding -Wall -Wextra -fno-exceptions -mcmodel=large -mno-red-zone -fno-rtti -g -DOS_H_HELPER_FUNCTIONS -DARCH_64 -DARCH_X86_64 -DARCH_X86_COMMON -std=c++11 -Wno-frame-address -DDEBUG_BUILD   -O3
x86_64-elf-gcc -T linker64.ld -o bin/os/kernel -ffreestanding -nostdlib bin/os/kernel_x86_64.o bin/os/kernel.o -lgcc -g -mno-red-zone -z max-page-size=0x1000
cp bin/os/kernel bin/os/kernel_symbols
x86_64-elf-strip --strip-all bin/os/kernel

rm bin/os/*.o
