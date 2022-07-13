CC=sdcc
RM=rm
PACKIHX=packihx
MAKEBIN=makebin
TARGET?=VTX_S
# The chip seems to be:
# https://www.keil.com/dd/chip/3196.htm
# Need to find the correct configuration options to make this compile
SDCC_OPTS:=-mmcs51 --iram-size 256 --code-size 65536 --stack-auto
CFLAGS=$(SDCC_OPTS) -D$(TARGET)
LDFLAGS=$(SDCC_OPTS)

SRCS= \
  src/mcu.c \
  src/camera.c \
  src/dm6300.c \
  src/global.c \
  src/hardware.c \
  src/i2c.c \
  src/i2c_device.c \
  src/isr.c \
  src/lifetime.c \
  src/monitor.c \
  src/msp_displayport.c \
  src/print.c \
  src/rom.c \
  src/sfr_ext.c \
  src/smartaudio_protocol.c \
  src/spi.c \
  src/uart.c

_MAKE_BUILD_DIR:=$(shell mkdir -p build)
OBJ_PATHS=$(subst src/,build/,$(SRCS))
OBJS=$(subst .c,.rel,$(OBJ_PATHS))

all: $(TARGET).bin

$(TARGET).ihx: $(OBJS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS)

%.hex: %.ihx
	$(PACKIHX) $< > $@

%.bin: %.hex
	$(MAKEBIN) -p $< $@

$(OBJS): build/%.rel : src/%.c
	$(CC) $(CFLAGS) -o build/ -c $<

clean:
	$(RM) -rf build/

distclean: clean
	$(RM) -f $(TARGET).*
