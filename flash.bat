@echo off
REM STM32F103 Blue Pill - Build and Flash Script
REM Uses the same structure as the Makefile: sources in src/, HAL in HAL/,
REM object files and output in build/

set CFLAGS=-mcpu=cortex-m3 -mthumb -mfloat-abi=soft -ffunction-sections -fdata-sections -O0 -g -Wall -I.
set LDFLAGS=-mcpu=cortex-m3 -mthumb -mfloat-abi=soft -specs=nosys.specs -specs=nano.specs -Tstm32f103.ld -Wl,--gc-sections,--print-memory-usage,--entry=Reset_Handler

echo ========================================
echo  STM32 Blue Pill - Build
echo ========================================

REM Create build directories
if not exist build\src     mkdir build\src
if not exist build\HAL\IO  mkdir build\HAL\IO
if not exist build\HAL\ISR mkdir build\HAL\ISR
if not exist build\HAL\UART mkdir build\HAL\UART
if not exist build\HAL\SPI mkdir build\HAL\SPI
if not exist build\HAL\I2C mkdir build\HAL\I2C
if not exist build\HAL\ADC mkdir build\HAL\ADC
if not exist build\HAL\DAC mkdir build\HAL\DAC
if not exist build\Drivers\16x2_LCD mkdir build\Drivers\16x2_LCD
if not exist build\Drivers\7_Segments mkdir build\Drivers\7_Segments
if not exist build\Drivers\I2C_OLED_Display mkdir build\Drivers\I2C_OLED_Display
if not exist build\Drivers\Keypad mkdir build\Drivers\Keypad
if not exist build\Drivers\PN532_NFC mkdir build\Drivers\PN532_NFC

REM Compile application sources
echo [CC] src/main.c
arm-none-eabi-gcc %CFLAGS% -c src/main.c -o build/src/main.o
if errorlevel 1 ( echo FAILED: src/main.c & pause & exit /b 1 )

echo [CC] src/startup.c
arm-none-eabi-gcc %CFLAGS% -c src/startup.c -o build/src/startup.o
if errorlevel 1 ( echo FAILED: src/startup.c & pause & exit /b 1 )

REM Compile HAL drivers
echo [CC] HAL/IO/IO.c
arm-none-eabi-gcc %CFLAGS% -c HAL/IO/IO.c -o build/HAL/IO/IO.o
if errorlevel 1 ( echo FAILED: HAL/IO & pause & exit /b 1 )

echo [CC] HAL/ISR/ISR.c
arm-none-eabi-gcc %CFLAGS% -c HAL/ISR/ISR.c -o build/HAL/ISR/ISR.o
if errorlevel 1 ( echo FAILED: HAL/ISR & pause & exit /b 1 )

echo [CC] HAL/UART/UART.c
arm-none-eabi-gcc %CFLAGS% -c HAL/UART/UART.c -o build/HAL/UART/UART.o
if errorlevel 1 ( echo FAILED: HAL/UART & pause & exit /b 1 )

echo [CC] HAL/SPI/SPI.c
arm-none-eabi-gcc %CFLAGS% -c HAL/SPI/SPI.c -o build/HAL/SPI/SPI.o
if errorlevel 1 ( echo FAILED: HAL/SPI & pause & exit /b 1 )

echo [CC] HAL/I2C/I2C.c
arm-none-eabi-gcc %CFLAGS% -c HAL/I2C/I2C.c -o build/HAL/I2C/I2C.o
if errorlevel 1 ( echo FAILED: HAL/I2C & pause & exit /b 1 )

echo [CC] HAL/ADC/ADC.c
arm-none-eabi-gcc %CFLAGS% -c HAL/ADC/ADC.c -o build/HAL/ADC/ADC.o
if errorlevel 1 ( echo FAILED: HAL/ADC & pause & exit /b 1 )

echo [CC] HAL/DAC/DAC.c
arm-none-eabi-gcc %CFLAGS% -c HAL/DAC/DAC.c -o build/HAL/DAC/DAC.o
if errorlevel 1 ( echo FAILED: HAL/DAC & pause & exit /b 1 )

REM Compile Device Drivers
echo [CC] Drivers/16x2_LCD/LCD.c
arm-none-eabi-gcc %CFLAGS% -c Drivers/16x2_LCD/LCD.c -o build/Drivers/16x2_LCD/LCD.o
if errorlevel 1 ( echo FAILED: Drivers/LCD & pause & exit /b 1 )

echo [CC] Drivers/7_Segments/SEG7.c
arm-none-eabi-gcc %CFLAGS% -c Drivers/7_Segments/SEG7.c -o build/Drivers/7_Segments/SEG7.o
if errorlevel 1 ( echo FAILED: Drivers/SEG7 & pause & exit /b 1 )

echo [CC] Drivers/I2C_OLED_Display/OLED.c
arm-none-eabi-gcc %CFLAGS% -c Drivers/I2C_OLED_Display/OLED.c -o build/Drivers/I2C_OLED_Display/OLED.o
if errorlevel 1 ( echo FAILED: Drivers/OLED & pause & exit /b 1 )

echo [CC] Drivers/Keypad/Keypad.c
arm-none-eabi-gcc %CFLAGS% -c Drivers/Keypad/Keypad.c -o build/Drivers/Keypad/Keypad.o
if errorlevel 1 ( echo FAILED: Drivers/Keypad & pause & exit /b 1 )

echo [CC] Drivers/PN532_NFC/PN532.c
arm-none-eabi-gcc %CFLAGS% -c Drivers/PN532_NFC/PN532.c -o build/Drivers/PN532_NFC/PN532.o
if errorlevel 1 ( echo FAILED: Drivers/PN532 & pause & exit /b 1 )

REM Link
echo [LD] build/firmware.elf
arm-none-eabi-gcc build/src/main.o build/src/startup.o build/HAL/IO/IO.o build/HAL/ISR/ISR.o build/HAL/UART/UART.o build/HAL/SPI/SPI.o build/HAL/I2C/I2C.o build/HAL/ADC/ADC.o build/HAL/DAC/DAC.o build/Drivers/16x2_LCD/LCD.o build/Drivers/7_Segments/SEG7.o build/Drivers/I2C_OLED_Display/OLED.o build/Drivers/Keypad/Keypad.o build/Drivers/PN532_NFC/PN532.o %LDFLAGS% -o build/firmware.elf
if errorlevel 1 ( echo Linking failed! & pause & exit /b 1 )

arm-none-eabi-objcopy -O binary build/firmware.elf build/firmware.bin
if errorlevel 1 ( echo Binary conversion failed! & pause & exit /b 1 )

arm-none-eabi-size build/firmware.elf

echo.
echo ========================================
echo  Flashing to device...
echo ========================================
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program build/firmware.elf verify reset exit"

if errorlevel 1 (
    echo Flash failed!
    pause
    exit /b 1
)

echo.
echo Done!
pause
