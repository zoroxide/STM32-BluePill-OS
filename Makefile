# STM32F103 Blue Pill Makefile
TARGET  = build/firmware

# Directories
SRC_DIR = src
HAL_DIR = HAL
DRV_DIR = Drivers
BLD_DIR = build

# Sources (only compile main.c and startup.c; os.c and apps/ are #included by main.c)
SRC_SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/startup.c
HAL_SOURCES = $(wildcard $(HAL_DIR)/IO/*.c)   \
              $(wildcard $(HAL_DIR)/ISR/*.c)  \
              $(wildcard $(HAL_DIR)/UART/*.c) \
              $(wildcard $(HAL_DIR)/SPI/*.c)  \
              $(wildcard $(HAL_DIR)/I2C/*.c)  \
              $(wildcard $(HAL_DIR)/ADC/*.c)  \
              $(wildcard $(HAL_DIR)/DAC/*.c)
DRV_SOURCES = $(wildcard $(DRV_DIR)/16x2_LCD/*.c)        \
              $(wildcard $(DRV_DIR)/7_Segments/*.c)       \
              $(wildcard $(DRV_DIR)/I2C_OLED_Display/*.c) \
              $(wildcard $(DRV_DIR)/Keypad/*.c)           \
              $(wildcard $(DRV_DIR)/RC522_RFID/*.c)

ALL_SOURCES = $(SRC_SOURCES) $(HAL_SOURCES) $(DRV_SOURCES)
OBJECTS     = $(patsubst %.c,$(BLD_DIR)/%.o,$(ALL_SOURCES))

# Tools
PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
LD      = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
SIZE    = $(PREFIX)size

# Flags
CFLAGS  = -mcpu=cortex-m3 -mthumb -mfloat-abi=soft
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -O0 -g -Wall
CFLAGS += -I.

LDFLAGS  = -mcpu=cortex-m3 -mthumb -mfloat-abi=soft
LDFLAGS += -specs=nosys.specs -specs=nano.specs
LDFLAGS += -Tstm32f103.ld
LDFLAGS += -Wl,--gc-sections,--print-memory-usage,--entry=Reset_Handler

# Build targets
all: $(TARGET).elf $(TARGET).bin

$(TARGET).elf: $(OBJECTS)
	@mkdir -p $(dir $@)
	$(LD) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Pattern rule: compile any .c into build/<path>.o
$(BLD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BLD_DIR)

flash: $(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
		-c "program $(TARGET).elf verify reset exit"

.PHONY: all clean flash

