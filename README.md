# SPI Sniffer
SPI Sniffer is small tool made on STM32F1Discovery laying around. It comes handy when you are debugging SPI communications and you just don't know if it is working properly (we all know that state of mind "why the hell that thing doesn't reply"). 

## Usage
Using AC6 System Workbench open project with std periph lib and set SPI according you application (I am talking about CPOL, CPHA and MSB First). Then download code into board. Connections are following:

  - UART TX (PA9) -> your USB-UART convertor's RX
  - GND -> your USB-UART convertor's GND
  - SPI NSS (PB12) -> your application NSS/CS/whatever
  - SPI SCK (PB13) -> your application SCK
  - SPI MISO (PB14) -> you application MISO/MOSI, depends what are you going to listen

It works in this way: when input buffer fills (32B), sniffing stops and buffer flushes to UART. After that process repeats. Reason is SPI is way faster than UART, so there is no way how to get data out at real time. Sorry. 

Happy SPI debuging!
