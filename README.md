# 📡ESP32-Samsung-BLE-Spammer
A tool that spams samsung BLE devices.

# 👍Recommended
- Use those ESP32 boards which has at least `4MB` flash memory.

# 📦Requirements
- `1` NodeMCU ESP-32S V1.1 38-Pins
- `1` Micro-B USB / Type-C USB Cable with data transfer support

# ⚙️Setup in Samsung Devices
1. Go to `Settings`.
2. Then, go to `Connections`.
3. Then, go to `More connection settings`.
4. Then, go to `Nearby device scanning`.
5. Enable the option.
6. Done!

# ⚙️Setup `ESP32-Samsung-BLE-Spammer`
1. Download `Arduino IDE 2.X.X` from [here](https://www.arduino.cc/en/software/) according to your Operating System.
2. Install it.
3. Go to `File` → `Preferences` → `Additional Boards Manager URLs`.
4. Paste the following link :
```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```
5. Click on `OK`.
6. Go to `Tools` → `Board` → `Board Manager`.
7. Wait for sometimes and search `esp32` by `Espressif Systems`.
8. Simply install latest version.
   - Wait for sometime and after that it is installed.
9. Done! Arduino IDE with required boards and libraries is ready.
10. Download or Clone the Repository.
11. Open the folder and then open `ESP32-Samsung-BLE-Spammer` folder and just double click on `ESP32-Samsung-BLE-Spammer.ino` file.
   - It opens in Arduino IDE.
15. Compile the code.
16. Select the correct board from the `Tools` → `Board` → `esp32`.
  - It is generally `NodeMCU-32S`.
17. Select the correct port number of that board.
18. Upload the code.
   - Wait for sometime to upload.
19. Done! It starts spamming automatically.
