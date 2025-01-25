This repository provides code for interfacing with an ESP32 and a PN532 NFC module using the I2C protocol. It includes functionality for reading from and writing to an NFC card.

The Write function writes encrypted data to an NFC card via the Serial Port. It uses a simple XOR encryption method, which is both symmetric and reversible. This function has been integrated into a C# authenticator application.

The Read function retrieves information from the NFC card and the Flask server, verifying if there is a match with a specific UID. It employs the same decryption algorithm to process the data.

This project was developed as part of a university assignment. Suggestions or improvements to the code are welcome—feel free to contact me at markcapatina5@gmail.com.
