EXT_C=gcc
EXT_L=gcc
IN_C=i686-elf-gcc
IN_L=i686-elf-gcc
EMU=qemu-system-i386
NAME=strayos

EMU_F=-monitor stdio -m 2G -cpu max -drive format=raw,file=$(NAME).bin # -D dbg.txt -d cpu_reset
EXT_CF=-Wall -Wextra -O2 -std=gnu99 -I ../lib-common/inc -Werror=conversion
EXT_LF=-Wall -Wextra -O2 -lgcc
IN_CF=-Wall -Wextra -O0 -ffreestanding -std=gnu99 -I ../lib-common/inc -Werror=conversion
IN_LF=-Wall -Wextra -O0 -ffreestanding -nostdlib -lgcc

SC_LIB_COMMON=$(shell find ../lib-common -name *.c)
SC_UT_CHADFS=$(shell find ../ut-chadfs -name *.c)
OC_LIB_COMMON_EXT=$(addsuffix .ext.o, $(SC_LIB_COMMON))
OC_UT_CHADFS=$(addsuffix .ext.o, $(SC_UT_CHADFS))

all:
	$(MAKE) build-all
	$(MAKE) soft-clean
	$(MAKE) run

build-all:
	$(MAKE) build-lib-common
	$(MAKE) build-ut-chadfs

	$(MAKE) build-strayldr
	mv ./pre.bin ./$(NAME).bin

build-strayldr:
	nasm -fbin ../strayldr/pre/pre.asm -o pre.bin

build-lib-common:
	$(MAKE) $(OC_LIB_COMMON_EXT)

build-ut-chadfs:
	$(MAKE) $(OC_UT_CHADFS)
	$(EXT_L) $(OC_UT_CHADFS) $(OC_LIB_COMMON_EXT) $(EXT_LF) -o ut-chadfs

%.c.ext.o:		%.c
	$(EXT_C) -c $< $(EXT_CF) -o $@

%.c.in.o:		%.c
	$(IN_C) -c $< $(IN_CF) -o $@

soft-clean:
	rm -f $(OC_LIB_COMMON_EXT) $(OC_UT_CHADFS) pre.bin

clean:
	$(MAKE) soft-clean
	rm -f $(NAME) ut-chadfs

run:
	$(EMU) $(EMU_F)