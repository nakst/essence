clear
echo "Build started..."

export DRIVE_RAW=drive
export DRIVE=/dev/loop0
export PARTITION_OFFSET=2048
export PARTITION_SIZE=129024
export OS_FOLDER=os

echo "Building utilities..."
g++ util/esfs.cpp -o esfs -g
chmod +x esfs

echo "Formatting drive..."
./esfs $DRIVE_RAW $PARTITION_OFFSET format $((512 * $PARTITION_SIZE)) MyVolume

# echo "Setting up loop device..."
# # Setup the first partition on the drive as the loop device
# sudo losetup $DRIVE $DRIVE_RAW -o $((512 * $PARTITION_OFFSET))

# echo "Mounting loop device..."
# # Mount the loop device
# sudo mount $DRIVE mount
# sudo chmod 777 mount

echo "Creating MBR..."
# Build and install the MBR
nasm -fbin boot/mbr.s -obin/mbr
dd if=bin/mbr of=$DRIVE_RAW bs=436 count=1 conv=notrunc status=none

echo "Installing bootloader..."
# Build and install the bootloader at the start of the partition
nasm -fbin boot/ext2-stage1.s -obin/stage1
dd if=bin/stage1 of=$DRIVE_RAW bs=512 count=2 conv=notrunc seek=$PARTITION_OFFSET status=none
nasm -fbin boot/ext2-stage2.s -obin/$OS_FOLDER/boot

echo "Compiling the kernel and userspace programs..."
# Build and link the kernel
./compile.sh

echo "Creating the bootfsid file..."
# Create a bootfsid file
echo -n 1234 > bin/os/bootfsid

echo "Removing temporary files..."
rm bin/mbr
rm bin/stage1

echo "Copying the files to the drive..."
./esfs $DRIVE_RAW $PARTITION_OFFSET import / bin/
./esfs $DRIVE_RAW $PARTITION_OFFSET import /os/ res/
# # Copy the bin directory to the mount
# cp -r bin/* mount/
# cp -r res/* mount/os/

# echo "Unmounting the loop device..."
# # Unmount the loop device
# sudo umount $DRIVE
# sudo losetup --detach $DRIVE

echo "Build successful."
