# pico-donut

This is a port of the donut.c version from:

https://www.a1k0n.net/2025/01/10/tiny-tapeout-donut.html

I have basically mimic the behaviour of the demo, fps is about 16 fps in a pico2 displayed to st7789 (240x240) but rendering donut at lower 120x120 resolution.

Each core (core0 and core1) is responsible of half screen and core0 updates st7789.

This is the result: 

![Pico explorer with st7789 display](resources/pico-explorer.jpeg)

![Video](resources/pico-explorer.mp4)

Install Intruccions:

1. Import project into raspberry pico vs code extension

2. It will run straightforward with raspberry pico explorer from pimoroni

Controls:

4 buttons to change dinamically the cordic iterations to show facets (faster) or a continuos donut.

