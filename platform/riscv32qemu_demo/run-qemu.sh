echo "hit Ctrl+a x to quit"
qemu-system-riscv32 -machine virt -nographic -bios image.bin -serial mon:stdio
