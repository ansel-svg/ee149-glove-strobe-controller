# ee149-glove-strobe-controller
EE149 Final Project

A wearable glove that controls an RGB DMX strobe light using hand orientation and grip. An operator wearing the glove tilts their hand to change the light's brightness and color in real time. A secondary mode activated by closing the fist switches the same hand movements to control strobe rate and white intensity. The system uses two ESP32 microcontrollers communicating wirelessly over ESP-NOW: one on the glove reading sensors, and one as a stationary receiver that streams mapped DMX channel values over USB serial to a laptop, which outputs DMX512 to the light fixture through a USB-to-DMX adapter.
