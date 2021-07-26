include ../nuttx-export-10.1.0-RC1/scripts/Make.defs

# Long calls are need to call from RAM into FLASH

# ARCHCFLAGS += -mlong-calls
# ARCHWARNINGS = -Wall -Wstrict-prototypes -Wshadow -Wundef
# ARCHOPTIMIZATION = -Os -fno-strict-aliasing -fno-strength-reduce -fomit-frame-pointer
# ARCHINCLUDES = -I. -isystem  ../nuttx-export-10.1.0-RC1/include


ARCHCFLAGS += -mlong-calls
ARCHWARNINGS = -Wall -Wstrict-prototypes -Wshadow -Wundef
ARCHOPTIMIZATION = -Os -mcpu=cortex-m4 -mthumb -fno-strict-aliasing -fno-strength-reduce -fomit-frame-pointer
ARCHINCLUDES = -I. -isystem  ../nuttx-export-10.1.0-RC1/include/ -I../../../apps/include/ -I../../../apps/applications/ledcontrol

CFLAGS = $(ARCHCFLAGS) $(ARCHWARNINGS) $(ARCHOPTIMIZATION) $(ARCHINCLUDES) -pipe

CROSSDEV = arm-none-eabi-
CC = $(CROSSDEV)gcc
LD = $(CROSSDEV)ld
STRIP = $(CROSSDEV)strip --strip-unneeded
CP = $(CROSSDEV)objcopy
CPFLAGS = -O ihex

# Setup up linker command line options

LDRELFLAGS = -r

LDELFFLAGS = -r -e main
LDELFFLAGS += -T defines.ld -T gnu-elf.ld

# This might change in a different environment

OBJEXT ?= .o

# This is the generated ELF program

BIN = ledcontrol
REL = ledcontrol.r

# These are the sources files that we use

SRCS = led_ctrl_main.c
OBJS = $(SRCS:.c=$(OBJEXT))

# Build targets

all: $(BIN)
.PHONY: clean

$(OBJS): %$(OBJEXT): %.c
	$(CC) -c $(CFLAGS) -o $@ $<

System.map: ../nuttx-export-10.1.0-RC1/System.map
	cat ../nuttx-export-10.1.0-RC1/System.map | sed -e "s/\r//g" >System.map

$(REL): $(OBJS)
	$(LD) $(LDRELFLAGS) -o $@ $<

defines.ld: System.map $(REL)
	./mkdefines.sh System.map "$(REL)" >defines.ld

$(BIN): defines.ld $(REL)
	$(LD) $(LDELFFLAGS) -o $@ $(REL)
	$(STRIP) $(REL)

hex:
	$(CP) $(CPFLAGS) $(BIN) $(BIN).hex

clean:
	rm -f $(BIN)
	rm -f $(REL)
	rm -f defines.ld
	rm -f System.map
	rm -f *.o
	rm -f *.hex
