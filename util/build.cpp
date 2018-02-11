#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

char buffer[4096];

void Build(bool enableOptimisations) {
	srand(time(NULL));
	printf("Build started...\n");

	char installationIdentifier[48];

	for (int i = 0; i < 48; i++) {
		if (i == 47) {
			installationIdentifier[i] = 0;
		} else if ((i % 3) == 2) {
			installationIdentifier[i] = '-';
		} else {
			char *hex = (char *) "0123456789ABCDEF";
			installationIdentifier[i] = hex[rand() % 16];
		}
	}

	printf("Generating tags...\n");
	system("ctags -R .");

	printf("Building utilities...\n");
	system("g++ util/esfs.cpp -o esfs -g");
	system("chmod +x esfs");

	printf("Creating output directories...\n");
	system("mkdir -p bin/os");

	printf("Creating MBR...\n");
	system("nasm -fbin boot/x86/mbr.s -obin/mbr");
	system("dd if=bin/mbr of=drive bs=436 count=1 conv=notrunc status=none");

	printf("Installing bootloader...\n");
	system("nasm -fbin boot/x86/esfs-stage1.s -obin/stage1");
	system("dd if=bin/stage1 of=drive bs=512 count=1 conv=notrunc seek=2048 status=none");
	system("nasm -fbin boot/x86/loader.s -obin/stage2 -Pboot/x86/esfs-stage2.s");
	system("dd if=bin/stage2 of=drive bs=512 count=7 conv=notrunc seek=2049 status=none");

	printf("Compiling the kernel and userspace programs...\n");

	if (enableOptimisations) {
		system("./compile.sh \"-O3\" \"-O3 -DDEBUG_BUILD\"");
	} else {
		system("./compile.sh \"\" \"-DDEBUG_BUILD\"");
	}

	printf("Removing temporary files...\n");
	system("rm bin/mbr");
	system("rm bin/stage1");

	printf("Formatting drive...\n");
	system("./esfs drive 2048 format 66060288 \"Essence HD\" bin/os/kernel");
	sprintf(buffer, "./esfs drive 2048 set-installation %s", installationIdentifier);
	system(buffer);

	printf("Copying files to the drive...\n");
	system("./esfs drive 2048 import / bin/");
	system("./esfs drive 2048 import /os/ res/");

	printf("Build complete.\n");
}

#define DRIVE_ATA (0)
#define DRIVE_AHCI (1)
#define LOG_VERBOSE (0)
#define LOG_NORMAL (1)
#define LOG_NONE (2)
#define EMULATOR_QEMU (0)
#define EMULATOR_BOCHS (1)
#define EMULATOR_VIRTUALBOX (2)

void Run(int emulator, int drive, int memory, int cores, int log, bool gdb) {
	switch (emulator) {
		case EMULATOR_QEMU: {
			sprintf(buffer, "qemu-system-x86_64 %s -m %d -s %s -smp cores=%d %s", 
					drive == DRIVE_ATA ? "-drive file=drive,format=raw,media=disk,index=0" : 
						"-drive file=drive,if=none,id=mydisk,format=raw,media=disk,index=0 -device ich9-ahci,id=ahci -device ide-drive,drive=mydisk,bus=ahci.0",
					memory, gdb ? "-S" : "", cores,
					log == LOG_VERBOSE ? "-d cpu_reset,int  > log.txt 2>&1" : (log == LOG_NORMAL ? " > log.txt 2>&1" : " > /dev/null 2>&1"));
			system(buffer);
		} break;

		case EMULATOR_BOCHS: {
			system("bochs -f bochs-config -q");
		} break;

		case EMULATOR_VIRTUALBOX: {
			if (!access("vbox.vdi", F_OK) == -1) {
				printf("Error: vbox.vdi does not exist.\n");
				return;
			}

			system("rm vbox.vdi");
			system("VBoxManage showmediuminfo vbox.vdi | grep \"^UUID\" > vmuuid.txt");
			FILE *f = fopen("vmuuid.txt", "r");
			char uuid[37];
			uuid[36] = 0;
			fread(uuid, 1, 16, f);
			fread(uuid, 1, 36, f);
			fclose(f);
			system("rm vmuuid.txt");
			sprintf(buffer, "VBoxManage convertfromraw drive vbox.vdi --format VDI --uuid %s", uuid);
			system(buffer);
		} break;
	}
}

int main(int argc, char **argv) {
	char *prev = nullptr;
	printf("Essence Build System\nPress Ctrl-C to exit.\nEnter 'help' to get a list of commands.\n");

	while (true) {
		char *l = nullptr;
		size_t pos = 0;
		printf("\n> ");
		getline(&l, &pos, stdin);

		if (strlen(l) == 1) {
			l = prev;
			if (!l) l = (char *) "help";
			printf("(%s)\n", l);
		} else {
			l[strlen(l) - 1] = 0;
		}

		if (0 == strcmp(l, "build") || 0 == strcmp(l, "b")) {
			Build(false);
		} else if (0 == strcmp(l, "optimise") || 0 == strcmp(l, "o")) {
			Build(true);
		} else if (0 == strcmp(l, "test") || 0 == strcmp(l, "t")) {
			Build(false);
			Run(EMULATOR_QEMU, DRIVE_AHCI, 64, 4, LOG_NORMAL, false);
		} else if (0 == strcmp(l, "low-memory")) {
			Build(false);
			Run(EMULATOR_QEMU, DRIVE_AHCI, 32, 4, LOG_NORMAL, false);
		} else if (0 == strcmp(l, "debug") || 0 == strcmp(l, "d")) {
			Build(false);
			Run(EMULATOR_QEMU, DRIVE_AHCI, 64, 1, LOG_NORMAL, true);
		} else if (0 == strcmp(l, "debug-smp")) {
			Build(false);
			Run(EMULATOR_QEMU, DRIVE_AHCI, 64, 4, LOG_NORMAL, true);
		} else if (0 == strcmp(l, "vbox") || 0 == strcmp(l, "v")) {
			Build(true);
			Run(EMULATOR_VIRTUALBOX, 0, 0, 0, 0, false);
		} else if (0 == strcmp(l, "exit") || 0 == strcmp(l, "x")) {
			break;
		} else if (0 == strcmp(l, "compile") || 0 == strcmp(l, "c")) {
			system("./compile.sh");
		} else if (0 == memcmp(l, "lua ", 4) || 0 == memcmp(l, "l ", 2)) {
			sprintf(buffer, "lua -e \"print(%s)\"", 1 + strchr(l, ' '));
			system(buffer);
		} else if (0 == memcmp(l, "python ", 7) || 0 == memcmp(l, "p ", 2)) {
			sprintf(buffer, "python -c \"print(%s)\"", 1 + strchr(l, ' '));
			system(buffer);
		} else if (0 == strcmp(l, "help") || 0 == strcmp(l, "h")) {
			printf("(b) build - Unoptimised build\n");
			printf("(o) optimise - Optimised build\n");
			printf("(t) test - Qemu (SMP/AHCI/64MB)\n");
			printf("( ) low-memory - Qemu (SMP/AHCI/32MB)\n");
			printf("(d) debug - Qemu (AHCI/64MB/GDB)\n");
			printf("( ) debug-smp - Qemu (AHCI/64MB/GDB/SMP)\n");
			printf("(v) vbox - VirtualBox (optimised)\n");
			printf("(x) exit - Exit the build system.\n");
			printf("(h) help - Show the help prompt.\n");
			printf("(l) lua - Execute a Lua expression.\n");
			printf("(p) python - Execute a Lua expression.\n");
			printf("(c) compile - Compile the kernel and programs.\n");
		} else {
			printf("Unrecognised command '%s'. Enter 'help' to get a list of commands.\n", l);
		}

		if (prev != l) free(prev);
		prev = l;
	}

	return 0;
}
