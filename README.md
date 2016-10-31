# iATag_test
Implement state machine for ISO14443-3 Tag.
Note: Currently this tag state machine does not handle parity or CRC and assumes it will be handled by underlying hardware.
We also cannot support sending partial byte AC response as it needs special harware support.
The state machine does check for proper valid bits command. 
