clear
echo "Build started..."

# TARGET_FS="ext2"
TARGET_FS="esfs"
DRIVE_RAW=drive
DRIVE=/dev/loop0
PARTITION_OFFSET=2048
PARTITION_SIZE=129024
OS_FOLDER=os

echo "Building utilities..."
g++ util/esfs.cpp -o esfs -g
chmod +x esfs

echo "Creating MBR..."
# Build and install the MBR
nasm -fbin boot/x86/mbr.s -obin/mbr
dd if=bin/mbr of=$DRIVE_RAW bs=436 count=1 conv=notrunc status=none

echo "Installing bootloader..."
# Build and install the bootloader at the start of the partition
if [ "ext2" == "$TARGET_FS" ]; then
	nasm -fbin boot/x86/ext2-stage1.s -obin/stage1
	dd if=bin/stage1 of=$DRIVE_RAW bs=512 count=2 conv=notrunc seek=$PARTITION_OFFSET status=none
	nasm -fbin boot/x86/loader.s -obin/$OS_FOLDER/boot -Pboot/x86/ext2-stage2.s
else
	nasm -fbin boot/x86/esfs-stage1.s -obin/stage1
	dd if=bin/stage1 of=$DRIVE_RAW bs=512 count=1 conv=notrunc seek=$PARTITION_OFFSET status=none
	nasm -fbin boot/x86/loader.s -obin/stage2 -Pboot/x86/esfs-stage2.s
	dd if=bin/stage2 of=$DRIVE_RAW bs=512 count=7 conv=notrunc seek=$((1 + $PARTITION_OFFSET)) status=none
fi

echo "Compiling the kernel and userspace programs..."
# Build and link the kernel
./compile.sh

echo "Creating the bootfsid file..."
# Create a bootfsid file
echo -n 1234 > bin/os/bootfsid

echo "Removing temporary files..."
rm bin/mbr
rm bin/stage1

echo "Formatting drive..."
if [ "ext2" == "$TARGET_FS" ]; then
	cp drive2 drive
else
	./esfs $DRIVE_RAW $PARTITION_OFFSET format $((512 * $PARTITION_SIZE)) MyVolume bin/os/kernel
fi

if [ "ext2" == "$TARGET_FS" ]; then
	echo "Setting up loop device..."
	# Setup the first partition on the drive as the loop device
	sudo losetup $DRIVE $DRIVE_RAW -o $((512 * $PARTITION_OFFSET))

	echo "Mounting loop device..."
	# Mount the loop device
	sudo mount $DRIVE mount
	sudo chmod 777 mount
fi

echo "Copying the files to the drive..."
# Copy the bin directory to the mount
if [ "ext2" == "$TARGET_FS" ]; then
	sudo cp -r bin/* mount/
	sudo cp -r res/* mount/os/
else
	./esfs $DRIVE_RAW $PARTITION_OFFSET import / bin/
	./esfs $DRIVE_RAW $PARTITION_OFFSET import /os/ res/
fi

if [ "ext2" == "$TARGET_FS" ]; then
	echo "Unmounting the loop device..."
	# Unmount the loop device
	sudo umount $DRIVE
	sudo losetup --detach $DRIVE
fi

echo "Build successful."
