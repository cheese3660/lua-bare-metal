#!/bin/sh

set -e
make
mkdir -p iso/boot/grub
cp kernel ./iso/boot/kernel
cp $1 iso/boot/initrd
cat > iso/boot/grub/grub.cfg << EOF
menuentry "openos" {
    multiboot /boot/kernel
    initrd /boot/initrd
}
EOF
grub-mkrescue -o kernel.iso iso
echo "(ctrl+c to exit)"

qemu-system-i386 -cpu 486 -device isa-debug-exit -cdrom kernel.iso -serial stdio -audiodev pa,id=snd0 -machine pcspk-audiodev=snd0
