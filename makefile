PROJNAME:=keypad

BUILD:=build/
SRC:=src/

OBJS:=\
keypad

BUILDOBJS:=$(BUILD)$(OBJS).o
ASMOBJS:=$(BUILD)$(OBJS).s

CFLAGS:= -Wall -Os

.PHONY: directories

all: directories $(BUILD)$(PROJNAME).hex

directories: $(BUILD)

$(BUILD):
	mkdir -p $(BUILD)

asm: $(ASMOBJS)

$(BUILD)$(PROJNAME).hex: $(BUILDOBJS)
	avr-gcc -mmcu=atmega2560 $(BUILDOBJS) -o $(BUILD)$(PROJNAME)
	avr-objcopy -O ihex -R .eeprom $(BUILD)$(PROJNAME) $(BUILD)$(PROJNAME).hex

$(BUILD)%.o: $(SRC)%.c
	avr-gcc $(CFLAGS) -DF_CPU=16000000UL -mmcu=atmega2560 -c -o $@ $<

$(BUILD)%.s: $(SRC)%.c
	avr-gcc $(CFLAGS) -DF_CPU=16000000UL -mmcu=atmega2560 -S -o $@ $<


install: directories $(BUILD)$(PROJNAME).hex
	avrdude -c wiring -p ATMEGA2560 -P /dev/ttyACM* -b 115200 -D -U flash:w:$(BUILD)$(PROJNAME).hex

clean: 
	rm -rf $(BUILD)