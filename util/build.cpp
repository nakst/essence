#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>

#define MANIFEST_PARSER_LIBRARY
#include "manifest_parser.cpp"

bool acceptedLicense;
char compilerPath[4096];

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
	system("g++ util/esfs.cpp -o esfs -g -Wall");
	system("g++ util/manifest_parser.cpp -o manifest_parser -g -Wall");
	system("chmod +x esfs");
	system("chmod +x manifest_parser");

	printf("Creating output directories...\n");
	system("mkdir -p bin/OS");

	printf("Creating MBR...\n");
	system("nasm -fbin boot/x86/mbr.s -obin/mbr");
	system("dd if=partition_table of=drive bs=512 count=1 conv=notrunc status=none");
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
	system("./esfs drive 2048 format 66060288 \"Essence HD\" bin/OS/kernel");
	sprintf(buffer, "./esfs drive 2048 set-installation %s", installationIdentifier);
	system(buffer);

	printf("Copying files to the drive...\n");
	system("./esfs drive 2048 import / bin/");
	system("./esfs drive 2048 import /OS/ res/");

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
			if (access("vbox.vdi", F_OK) == -1) {
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

			system("VirtualBox --dbg --startvm OS");
		} break;
	}
}

void BuildCrossCompiler() {
	char yes[1024];

	{
		printf("\nThe build system could not detect a GCC cross compiler in your PATH.\n");
		printf("Have you already built a GCC cross compiler compatible with the build system?\n\n");

		printf("Type 'yes' or 'no'.\n");
		scanf("%s", yes);

		if (!strcmp(yes, "yes")) {
			printf("\nPlease enter the ABSOLUTE path of the bin/ folder, containing the binutils and GCC executables.\n");
			scanf("%s", compilerPath);

			printf("\nUpdating config...\n");
			return;
		}

		printf("\nThe build system will now automatically build a cross compiler for you.\n");
		printf("Make sure you are connected to the internet, and have the latest versions of the following programs:\n");
		printf("\t- GCC/G++\n");
		printf("\t- GNU Make\n");
		printf("\t- GNU Bison\n");
		printf("\t- Flex\n");
		printf("\t- GNU GMP\n");
		printf("\t- GNU MPFR\n");
		printf("\t- GNU MPC\n");
		printf("\t- Texinfo\n");
		printf("\t- curl\n");
		printf("\t- nasm\n");
		printf("\t- ctags\n");

		printf("\nMake sure you have at least 3GB of drive space available.\n");
		printf("The final installation will take up ~1GB.\n");
		printf("Approximately 100MB of source files will be downloaded.\n");
		printf("The full build may take over an hour on slower systems; on most modern systems, it should only take ~15 minutes.\n");

		printf("\nEnter the ABSOLUTE path of the folder which the cross compiler will be installed into:\n");
		char installationFolder[4096];
		scanf("%s", installationFolder);
		if (installationFolder[strlen(installationFolder) - 1] == '/') {
			installationFolder[strlen(installationFolder) - 1] = 0;
		}
		strcpy(compilerPath, installationFolder);
		strcat(compilerPath, "/bin");
		printf("\nType 'yes' to install the GCC compiler into '%s'.\n", installationFolder);
		char yes[1024];
		scanf("%s", yes);
		if (strcmp(yes, "yes")) goto fail;

		DIR *test = opendir("binutils-2.30");
		if (test) closedir(test);
		else {
			printf("Downloading Binutils source...\n");
			if (system("curl ftp://ftp.gnu.org/gnu/binutils/binutils-2.30.tar.xz > binutils.tar.xz")) goto fail;
			printf("Extracting Binutils source...\n");
			if (system("xz -d binutils.tar.xz")) goto fail;
			if (system("tar -xf binutils.tar")) goto fail;
			if (system("rm binutils.tar")) goto fail;
		}

		test = opendir("gcc-7.3.0");
		if (test) closedir(test);
		else {
			printf("Downloading GCC source...\n");
			if (system("curl ftp://ftp.gnu.org/gnu/gcc/gcc-7.3.0/gcc-7.3.0.tar.xz > gcc.tar.xz")) goto fail;
			printf("Extracting GCC source...\n");
			if (system("xz -d gcc.tar.xz")) goto fail;
			if (system("tar -xf gcc.tar")) goto fail;
			if (system("rm gcc.tar")) goto fail;
		}

		printf("Preparing build...\n");
		char path[65536];
		char *originalPath = getenv("PATH");
		if (strlen(originalPath) > 32768) {
			printf("PATH too long\n");
			goto fail;
		}
		strcpy(path, installationFolder);
		strcat(path, "/bin:");
		strcat(path, originalPath);
		setenv("PATH", path, 1);
		printf("PATH = %s\n", path);
		if (system("mkdir build-binutils")) goto fail;
		if (system("mkdir build-gcc")) goto fail;

		FILE *file;

		printf("Modifying source for libgcc w/o red zone...\n");
		file = fopen("gcc-7.3.0/gcc/config/i386/t-x86_64-elf", "w");
		if (!file) {
			printf("Couldn't modify source\n");
			goto fail;
		} else {
			fprintf(file, "MULTILIB_OPTIONS += mno-red-zone\nMULTILIB_DIRNAMES += no-red-zone\n");
			fclose(file);
		}
		file = fopen("temp.txt", "w");
		if (!file) {
			printf("Couldn't modify source\n");
			goto fail;
		} else {
			fprintf(file, "\ttmake_file=\"${tmake_file} i386/t-x86_64-elf\"\n");
			fclose(file);
		}
		if (system("sed -i '1460r temp.txt' gcc-7.3.0/gcc/config.gcc")) goto fail;
		if (system("rm temp.txt")) goto fail;

		char cmdbuf[65536];

		printf("Building binutils...\n");
		if (chdir("build-binutils")) goto fail;
		sprintf(cmdbuf, "../binutils-2.30/configure --target=x86_64-elf --prefix=\"%s\" --with-sysroot --disable-nls --disable-werror", installationFolder);
		if (system(cmdbuf)) goto fail;
		if (system("make")) goto fail;
		if (system("make install")) goto fail;
		if (chdir("..")) goto fail;

		printf("Building GCC...\n");
		if (chdir("build-gcc")) goto fail;
		sprintf(cmdbuf, "../gcc-7.3.0/configure --target=x86_64-elf --prefix=\"%s\" --enable-languages=c,c++ --without-headers --disable-nls", installationFolder);
		if (system(cmdbuf)) goto fail;
		if (system("make all-gcc")) goto fail;
		if (system("make all-target-libgcc")) goto fail;
		if (system("make install-gcc")) goto fail;
		if (system("make install-target-libgcc")) goto fail;
		if (chdir("..")) goto fail;

		printf("Cleaning up...\n");
		system("rm -r binutils-2.30/");
		system("rm -r gcc-7.3.0/");
		system("rm -rf build-binutils");
		system("rm -rf build-gcc");

		printf("Modifying headers...\n");
		sprintf(path, "%s/lib/gcc/x86_64-elf/7.3.0/include/mm_malloc.h", installationFolder);
		file = fopen(path, "w");
		if (!file) {
			printf("Couldn't modify header files\n");
			goto fail;
		} else {
			fprintf(file, "/*Removed*/\n");
			fclose(file);
		}

		printf("\nThe build has successfully completed.\n");
	}

	return;
	fail:;
	printf("\nThe build has failed. Please consult the documentation.\n");
	exit(0);
}

void LoadConfig(Token attribute, Token section, Token name, Token value, int event) {
	(void) attribute;
	(void) section;
	(void) name;
	(void) value;
	(void) event;

	if (CompareTokens(name, "accepted_license") && CompareTokens(value, "true")) {
		acceptedLicense = true;
	} else if (CompareTokens(name, "compiler_path")) {
		char path[65536];
		char *originalPath = getenv("PATH");
		if (strlen(originalPath) > 32768) {
			printf("Warning: PATH too long\n");
			return;
		}
		memcpy(compilerPath, value.text + 1, value.bytes - 2);
		strcpy(path, compilerPath);
		strcat(path, ":");
		strcat(path, originalPath);
		setenv("PATH", path, 1);
	}
}

int main(int argc, char **argv) {
	(void) argc;
	(void) argv;

	char *prev = nullptr;
	printf("Essence Build System\nPress Ctrl-C to exit.\n");

	{
		FILE *file = fopen("build_system_config.dat", "r");

		if (file) {
			fseek(file, 0, SEEK_END);
			size_t fileSize = ftell(file);
			fseek(file, 0, SEEK_SET);
			char *buffer = (char *) malloc(fileSize + 1);
			buffer[fileSize] = 0;
			fread(buffer, 1, fileSize, file);
			fclose(file);
			ParseManifest(buffer, LoadConfig);
			free(buffer);
		}
	}

	if (!acceptedLicense) {
		printf("\n=== Essence License ===\n\n");
		system("cat LICENSE.md");
		printf("\nType 'yes' to agree to the license, or press Ctrl-C to exit.\n");
		char yes[1024];
		scanf("%s", yes);
		if (strcmp(yes, "yes")) exit(0);
	}

	bool restart = false;

	if (system("x86_64-elf-gcc --version > /dev/null")) {
		BuildCrossCompiler();
		restart = true;
		printf("Please restart the build system.\n");
	}

	{
		FILE *file = fopen("build_system_config.dat", "w");
		fprintf(file, "[build_system]\naccepted_license = true;\n");
		if (strlen(compilerPath)) fprintf(file, "compiler_path = \"%s\";\n", compilerPath);
		fclose(file);
	}

	if (restart) {
		return 0;
	}

	{
		FILE *drive = fopen("drive", "r");
		if (!drive) {
			system("cp partition_table drive");
		} else {
			fclose(drive);
		}
	}

	printf("Enter 'help' to get a list of commands.\n");

	while (true) {
		char *l = nullptr;
		size_t pos = 0;
		printf("\n> ");
		getline(&l, &pos, stdin);

		if (strlen(l) == 1) {
			l = prev;
			if (!l) {
				l = (char *) malloc(5);
				strcpy(l, "help");
			}
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
		} else if (0 == strcmp(l, "ata")) {
			Build(false);
			Run(EMULATOR_QEMU, DRIVE_ATA, 64, 4, LOG_NORMAL, false);
		} else if (0 == strcmp(l, "bochs")) {
			Build(false);
			Run(EMULATOR_BOCHS, 0, 0, 0, 0, false);
		} else if (0 == strcmp(l, "test-without-smp") || 0 == strcmp(l, "t2")) {
			Build(false);
			Run(EMULATOR_QEMU, DRIVE_AHCI, 64, 1, LOG_NORMAL, false);
		} else if (0 == strcmp(l, "test-opt") || 0 == strcmp(l, "t4")) {
			Build(true);
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
		} else if (0 == strcmp(l, "vbox-without-opt") || 0 == strcmp(l, "v2")) {
			Build(false);
			Run(EMULATOR_VIRTUALBOX, 0, 0, 0, 0, false);
		} else if (0 == strcmp(l, "exit") || 0 == strcmp(l, "x") || 0 == strcmp(l, "quit") || 0 == strcmp(l, "q")) {
			break;
		} else if (0 == strcmp(l, "reset-config")) {
			system("rm build_system_config.dat");
			printf("Please restart the build system.\n");
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
			printf("(b ) build - Unoptimised build\n");
			printf("(o ) optimise - Optimised build\n");
			printf("(t ) test - Qemu (SMP/AHCI/64MB)\n");
			printf("(  ) ata - Qemu (SMP/ATA/64MB)\n");
			printf("(t2) test-without-smp - Qemu (AHCI/64MB)\n");
			printf("(t4) test-opt - Qemu (AHCI/64MB/optimised)\n");
			printf("(  ) bochs - Bochs\n");
			printf("(  ) low-memory - Qemu (SMP/AHCI/32MB)\n");
			printf("(d ) debug - Qemu (AHCI/64MB/GDB)\n");
			printf("(  ) debug-smp - Qemu (AHCI/64MB/GDB/SMP)\n");
			printf("(v ) vbox - VirtualBox (optimised)\n");
			printf("(  ) vbox-without-opt - VirtualBox (unoptimised)\n");
			printf("(x ) exit - Exit the build system.\n");
			printf("(h ) help - Show the help prompt.\n");
			printf("(l ) lua - Execute a Lua expression.\n");
			printf("(p ) python - Execute a Lua expression.\n");
			printf("(c ) compile - Compile the kernel and programs.\n");
			printf("(  ) reset-config - Reset the build system's config file.\n");
		} else {
			printf("Unrecognised command '%s'. Enter 'help' to get a list of commands.\n", l);
		}

		if (prev != l) free(prev);
		prev = l;
	}

	return 0;
}
