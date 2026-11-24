# IoT-Opgave5

ATmega328PB, 8 MHz, brugernavn `Lonneberg`. Arbejdskopi til Embedded Opgave 5.

## Opkobling

Pinvalgene nedenfor er forudsat i koden og skal kontrolleres mod din opstilling.

| Funktion | AVR-pin | Tilslutning |
| --- | --- | --- |
| Testdata ind, UART0 RX | PD0 / RXD0 | Afsenderens TX (boardets virtuelle COM-port) |
| Testsvar ud, UART1 TX | PB3 / TXD1 | CP2102 RX |
| SSD1306 SDA | PC4 / SDA0 | Display SDA |
| SSD1306 SCL | PC5 / SCL0 | Display SCL |
| Trykknap INT0 | PD2 | Knap mellem PD2 og GND; intern pull-up er aktiv |
| Sekund-LED | PB1 | LED med seriemodstand |
| Knap-LED | PB2 | LED med seriemodstand |

Forbind GND mellem board, CP2102 og display. Brug spændingsniveauer, der passer
til komponenterne. Displayet er 128x64, I2C-adresse 0x3C. Begge UART'er bruger
9600 baud, 8 databits, ingen paritet, 1 stopbit og ingen flow control.
PB3 bruges til UART1 og må derfor ikke samtidig bruges til rød LED.
F_CPU beskriver clocken; build-scriptet ændrer ikke boardets clock/fuses.

## Protokol og ringbuffer

1. Modtag `:` på UART0. Send `:Lonneberg:` på UART1.
2. Vent på `;` på UART0, før ringbufferen aktiveres.
3. Modtag binære bytes 0..255 gentaget. RX-interruptet lægger dem ved hovedet.
4. UART1 data-register-empty-interruptet sender fra halen uden polling/venteløkke.
5. Bufferen har 256 pladser, hvoraf 255 kan bruges. Den sidste plads skelner fuld
   fra tom. Indeks er 8 bit og wrapper automatisk.
6. Ved fuld buffer afbrydes testen. Allerede ventende bytes kasseres, og første
   afviste byte rapporteres decimalt som `;255;` (eksempel). Vent derefter på
   ASCII `-` eller `_`, og gå tilbage til at vente på `:`. Hardware-UART-fejl
   stopper også testen; her rapporteres den forventede byte som bedste estimat.

**Uklarhed i opgaveprotokollen:** `_` er både reset og dataværdien 95. Koden
behandler `_` som data, når 95 forventes, og ellers som reset. ASCII `-` er kun
reset i fejltilstanden; 45 sendes normalt videre under testen. Hvis appen sender
reset `_` netop når 95 forventes, kan signalerne ikke skelnes. Dette skal afklares
med lærerens app; en entydig løsning kræver framing, escaping eller anden kanal.
Tankestregen i PDF'ens punkt 7 er her fortolket som ASCII `-`.

UART0 bruges kun til inddata, UART1 kun til uddata. Der indsættes ingen debugtekst
i datastrømmen. Knapper og display påvirker ikke byteværdierne.

## Display og interrupts

| Linje (fra 0) | Indhold |
| --- | --- |
| 0 | Antal nedadgående knapflanker |
| 1 | Antal opadgående knapflanker |
| 2 | Sekunder siden opstart |
| 3 | Seneste fem bytes lagt i buffer, ældst til venstre, decimal 000..255 |
| 4 | Embedded Styrer !!! |
| 5 | Højeste bufferfyldning siden opstart |
| 6 | Antal overrun/UART-fejlhændelser siden opstart |
| 7 | Brugernavn og protokoltilstand |

Tilstande: 0 venter på kolon, 1 sender navn, 2 venter på semikolon,
3 datatest, 4 fejl/venter på reset. De to bufferstatistikker er ekstra funktion
til punkt 17. EEPROM-lagring er ikke implementeret; sekunder starter ved nul.
De tomme positioner på linje 3 vises som 000, indtil fem bytes er modtaget.

Timer1 bruger CTC, prescaler 256 og OCR1A=31249 ved 8 MHz. Sekunder tælles i ISR,
så displayarbejde ikke mister timer-ticks. PB1 toggler hvert sekund. INT0 reagerer
på begge flanker, tæller dem hver for sig og toggler PB2. Der er ingen software-
debounce: opgaven tæller elektriske flanker, så en mekanisk knap kan give flere
interrupts pr. tryk. Displayet opdateres i hovedløkken, højst én linje ad gangen;
alle linjer planlægges hvert sekund, knaplinjerne også ved knapinterrupt.
Displaydriverens font gengiver små bogstaver som store.

## Byg og indlæs

Åbn **denne mappe** i VS Code. Ctrl+Shift+B bygger med `build.ps1` og AVR-GCC.
Standardplacering er `C:\Udvikling\tools\avr\bin`; alternativt:

```powershell
.\build.ps1 -Toolchain 'C:\sti\til\avr\bin'
```

Resultater: `out/IoT-Opgave5.elf` og `out/IoT-Opgave5.hex`.
Scriptet bygger til ATmega328PB ved 8 MHz med advarsler som fejl.
AVR-GCC er hentet fra Arduinos officielle værktøjspakke, version
7.3.0-atmel3.6.1-arduino7, og downloadens SHA-256 er kontrolleret mod deres indeks.

MPLAB-konfigurationen er også oprettet til ATmega328PB med XC8 4.00 og
ATmega_DFP 3.6.299 som i lærerens eksempel. Den kræver en lokal MPLAB/XC8-
installation. Vælg eget mEDBG-board i MPLAB før programmering. Der er ikke kopieret
et fremmed toolSerial ind i projektet. Firmware er ikke indlæst på boardet herfra.

## Verifikation

AVR-GCC build/link er gennemført uden advarsler. UART-testen bruger den faktiske
`Source/UART/UART.c` med simulerede registre. Den tester handshake, data før start,
fire gennemløb af 0..255 inklusive kontroltegn, FIFO og wrap, seneste fem værdier,
reset, fuld buffer, decimal fejlrapport og hardware-overrun. Testen simulerer ikke
clock, elektriske signaler eller samtidige interrupts.

Kør fra en Visual Studio x64 Native Tools-kommandoprompt i projektmappen:

```bat
cl /nologo /std:c11 /W3 /I Tests\mocks Tests\uart_test.c /Fo:out\uart_test.obj /Fe:out\uart_test.exe
out\uart_test.exe
```

På boardet skal følgende stadig kontrolleres:

1. Kontrollér opkoblingen, indlæs HEX-filen, og se om displayet viser otte linjer.
2. Se at sekundtælleren stiger og PB1 toggler. Tryk/slip knappen, kontrollér begge
   flanketællere og PB2. Efter slip bør tællerne være omtrent ens.
3. Vælg korrekt COM-port til hver retning i lærerens app, begge på 9600 8N1.
4. Start handshake, og kontrollér modtagelsen af `:Lonneberg:`.
5. Kør flere gennemløb af 0..255 og kontrollér fejlfri tilbagesendelse, især
   værdierne 45, 58, 59, 95 og 255. Displayet skal fortsætte med at opdatere.
6. Afprøv appens reset og en overrun-test; afklar kontroltegnene med læreren.

## Displaydriverens kilde

`Include/Display/ssd1306_328pb.h` og `Source/Display/ssd1306_328pb.c` er fra:
https://github.com/smaldos64/h4pd100126_IOT_ATMega328PB_Opgave5_Display
Commit: `033db564b17372a562b5abae9d914c4c9023d628`.
Original forfatterangivelse er bevaret. Lokale ændringer: projektets 8 MHz-
definition indlæses før delay-headeren, ventetid på TWINT er begrænset,
og tekstudskrivning begrænses til skærmens 21 tegn pr. linje.
