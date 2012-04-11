The Make_CellCar project is Arduino code for the 'Cellphone Car Ignition' project in Make Magazine issue 30 (http://makezine.com/30).

Written for Arduino 1.0 and above.

Configuration:
Using Servo: Set 'useServo' to 1, 'servoNeutral' and 'servoPress' are angles on the servo for the standby (servoNeutral) and pressing (servoPress) positions. 'servoDelay' is the time in milliseconds that to hold the button to start the car. This is usually around 2 seconds (2000 milliseconds).

Using a relay board with 1 button: Set 'useServo' and 'use2Buttons' both to 0. Set 'button1Delay' to how long the start button should be pressed, in milliseconds. This is usually around 2 seconds (2000 milliseconds).

Using a relay board with 2 buttons: Set 'useServo' to 0 and 'use2Buttons' to 1. Set 'button1Delay' to the time the first button (usually 'Lock') should be pressed in milliseconds. This is usually something short like 500 milliseconds. Set 'button2Delay' to how long the start button should be pressed, in milliseconds. This is usually around 2 seconds (2000 milliseconds).


Phone numbers: The array 'numbers' contains phone numbers that allowed to start the car. Change the numbers in the array to add your numbers. If you need to change the number of numbers, change NUM_OF_NUMS and the number of elements in numbers accordingly.

Watchdog timer: Setting USE_WDT to 1 enables to watchdog timer, making the system more stable, but will only work properly with Ardunio Unos or other updated bootloader Arduinos. If you are not sure, leave set to 0.
Enabling this option on older Arduinos will cause the system to enter an infinite boot loop that is hard, but not impossible, to get out of.


Installation: After configuring, you can program the box using the Arduino IDE as you usually would.



