param([string]$Toolchain = 'C:\Udvikling\tools\avr\bin')
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    New-Item -ItemType Directory -Path out -Force | Out-Null
    & "$Toolchain\avr-gcc.exe" -mmcu=atmega328pb -DF_CPU=8000000UL -Os -std=gnu99 -Wall -Wextra -Werror -ffunction-sections -fdata-sections main.c Source/UART/UART.c Source/Timer/Timer.c Source/Display/ssd1306_328pb.c '-Wl,--gc-sections' -o out/IoT-Opgave5.elf
    if ($LASTEXITCODE -ne 0) { throw 'Compile/link failed' }
    & "$Toolchain\avr-objcopy.exe" -O ihex -R .eeprom out/IoT-Opgave5.elf out/IoT-Opgave5.hex
    if ($LASTEXITCODE -ne 0) { throw 'HEX generation failed' }
    & "$Toolchain\avr-size.exe" out/IoT-Opgave5.elf
} finally { Pop-Location }
