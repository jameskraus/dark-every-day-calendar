# The Every Day Calendar _with Dark Mode_

![dark mode](/images/dark_mode.gif)


I wanted an Every Day Calendar that would dim at night when I completed my goal of going to bed on time. As you can imagine, a bright board of glowing lights isn't the best for sleeping. Thankfully the Every Day Calendar has a helpful programming port and the default firmware. I added a few tweaks that make the calendar better for use in a bedroom near bedtime. Whenever you touch a day:

- The calendar dims the brightness
- The calendar only shows the beginning and end of streaks of days

After 8 hours the calendar will wake out of dark mode so you view all your touched days during the daytime. 

The calendar also temporarily shows all the touched days if you touch one while in dark mode. To exit dark mode early, just touch either of the brightness buttons.

The rest of the original Readme is below for reference. These changes to the board firmware are licensed under the MIT license too.

---

![edc](/images/edc.gif)
## Overview

This repository contains firmware and schematics of the [Every Day Calendar](https://www.kickstarter.com/projects/simonegiertz/the-every-day-calendar) project.

Please note that this is a finished project. The files are provided as is and we will not be maintaining this repository.


## Installation

To download the firmware and schematic files, run the following command on your terminal:


    git clone https://gitlab.com/simonegiertz/the-every-day-calendar.git
## Firmware

### Connecting the Every Day Calendar to the Arduino IDE

1. Power on the Every Day Calendar with a 5V adapter and connect the calendar to your computer using a USB Type-B to USB Typ-A cable.

2. Open the Arduino IDE

   - Configure the board type to **Arduino Pro or Pro Mini**

     ![board_config](/images/board_config.png)

   - Configure the processor and clock speed to **Atmega328P (3.3V, 8MHz)**

     ![clockconfig](/images/clockconfig.png)

### Installing Arduino Libraries

1. Copy the contents of this repository's *firmware > libraries* directory into your computer's Arduino libraries folder.
   
   - Typically that's located in **Documents/Arduino/libraries**
   
   **Note:** If more instruction is needed, follow Arduino's guide: https://www.arduino.cc/en/guide/libraries

To test and see if the custom libraries are working, you can run our sample code provided in the *firmware > sketches* directory of this Github repository. Simply upload one of the sample code files onto the calendar using the Arduino IDE, open the **Serial Monitor** and set the baud rate to **9600**.

![serialmonitor](/images/serialmonitor.png)

Once everything is working, you're ready to play with the Every Day Calendar! Have fun!

## License

The contents of this repository are released under the following licenses for hardware and software:

- Hardware: CC BY-SA 4.0: https://creativecommons.org/licenses/by-sa/4.0/
- Software: MIT License: https://opensource.org/licenses/MIT

