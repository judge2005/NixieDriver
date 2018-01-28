# NixieDriver
Low-level drivers for controlling nixie tubes running on an arduino platform.

These classes control various types of hardware to display digits on a nixie tube and provide a set of effects when moving from one digit to another.
This allows dislpay functions such as clocks, timers, etc. to be written idependently of the specific hardware they run on.

These classes have ony been compiled on ESPxxxx processors. They will probably need some modifications to run on other processes. The current set of drivers all use SPI to control Microchip HV chips and rely on a modified version of the SPI library called SPIInterrupts, also available on this repo.
