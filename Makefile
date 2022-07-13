CC=sdcc
RM=rm
TARGET?=VTX_S
CFLAGS=-mmcs51 -D$(TARGET)
CPPFLAGS=
LDFLAGS=
LDLIBS=

SRCS=src/camera.c src/global.c src/i2c.c src/isr.c src/mcu.c src/msp_displayport.c src/rom.c src/smartaudio_protocol.c src/uart.c src/dm6300.c src/hardware.c src/i2c_device.c  src/lifetime.c src/monitor.c src/print.c src/sfr_ext.c src/spi.c
OBJ_PATHS=$(subst src/,build/,$(SRCS))
OBJS=$(subst .c,.rel,$(OBJ_PATHS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS)

$(OBJS): build/%.rel : src/%.c
	mkdir -p build/
	$(CC) $(CFLAGS) -o build/ -c $<

clean:
	$(RM) -rf build/

distclean: clean
	$(RM) -f $(TARGET)
