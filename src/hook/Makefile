# Build the in-game hook blob (run inside WSL)
CC      = clang
LD      = ld.lld
CFLAGS  = -target x86_64-unknown-none-elf -Os -fPIE -ffreestanding -fno-builtin -nostdlib \
          -fno-stack-protector -fno-jump-tables -fno-asynchronous-unwind-tables -fvisibility=hidden \
          -mstackrealign -Wall -Wextra -Wno-unused-parameter -I../core
OBJS    = hooks.o menu.o ldproto.o
all: ../../build/ldhook.bin
hooks.o: hooks.c ldhook.h ../core/ldproto.h
	$(CC) $(CFLAGS) -c $< -o $@
menu.o: menu.c ldhook.h figures.h grid_nav.h grid_input.h
	$(CC) $(CFLAGS) -c $< -o $@
ldproto.o: ../core/ldproto.c ../core/ldproto.h
	$(CC) $(CFLAGS) -c $< -o $@
ldhook.elf: $(OBJS) link.ld
	$(LD) -T link.ld -pie --no-dynamic-linker -z norelro -o $@ $(OBJS)
../../build/ldhook.bin: ldhook.elf mkblob.py
	mkdir -p ../../build
	python3 mkblob.py $< $@
clean:
	rm -f *.o ldhook.elf ../../build/ldhook.bin
