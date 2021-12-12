# battery_monitor
12V battery monitoring

This project includes 2 boards for a service 12V Pb battery monitoring.

The main board monitors the battery voltage and current, using a microchip PAC1953 chip and a precision shunt resistor. It uses a pic18f with integrated CAN controller, and connects to a CAN bus network. Along with instant voltage and current values, the pic18f will be able to compute and report the battery SoC. As the PAC1953 has 3 channels, it will also be able to monitor other sources, like solar panel or the engine battery. The main battery will be monitored with an external, high current shunt resistor (100A/50mV) so one of the on-board shunt resitor won't be soldered.

The second board will convert the voltage across the high current shunt resistor to a voltage (between 0.5 and 4.5V) suitable to drive a pannel-mounted Osculati  +/-50A ampmeter(27.320.23). As I don't expect to have currents more than 20A here, I computed the gain so that the indicator range is +/-25A (that is, the indicated value will be twice the real value). Two potentiometers allows to adjust the 0 and the gain. This, along with an external analog voltmeter, provides a second, independant battery monitoring tool.
