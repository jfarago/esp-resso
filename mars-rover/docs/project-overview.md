# 3D-Printed Solar Mars Rover (ESP32 Build Guide)

## 1. Project Overview

A 6-wheel rocker-bogie Mars rover powered by an **ESP32** microcontroller, six **GA12-N20** micro metal gear motors, a **3.7V Li-ion/LiPo** battery with **CN3065 solar charging**, and dual **BH1750** light sensors for autonomous sun-seeking and battery recharging.

---

## 2. Bill of Materials (BOM)

| Category             | Component                        | Qty | Specs & Buying Notes                                                                                                                      |
| :-------------------- | :-------------------------------- | :---: | :----------------------------------------------------------------------------------------------------------------------------------------- |
| **Microcontroller**  | ESP32 Dev Board                  | 1   | Compact footprint recommended (**ESP32-C3 SuperMini**, **ESP32-S3 Mini**, or standard 30-pin **ESP32-WROOM-32**).                         |
| **Drive Motors**     | GA12-N20 DC Gear Motor           | 6   | **6V DC**, **60 RPM or 100 RPM**, all-metal gearbox, **3 mm × 10 mm D-shaft**, 2 solder tabs (no encoder). Buy all 6 from the same batch. |
| **Motor Driver**     | TB6612FNG or DRV8833 Module      | 1   | Dual H-bridge, **3.3V logic compatible**, ≥ 1.2A continuous per channel (replaces L293D).                                                 |
| **Battery**          | 3.7V Li-ion / LiPo Cell          | 1   | Protected **18650** (2000–3500 mAh) + holder, or protected **3.7V LiPo pouch** (1000–2000 mAh).                                           |
| **Solar Charger**    | CN3065 Solar LiPo Charger        | 1   | 4.4V–6V solar input window + USB backup port, 500 mA max charge current, 4.2V cutoff.                                                     |
| **Solar Panel**      | 6V Mini Epoxy Solar Panel        | 1   | **1.5W – 2.5W, 6V nominal** (`Voc ≤ 6.8V`; sized to fit top deck, ~110 × 136 mm or 80 × 80 mm).                                           |
| **Power Boost**      | DC-DC Step-Up Boost Module       | 1   | **MT3608** adjustable booster (pre-set to **5.2V–5.5V**) or fixed **5V 2A** step-up converter.                                            |
| **Light Sensors**    | BH1750 (GY-302) Module           | 2   | Digital I2C lux sensor (1–65,535 lux range for full sun without saturating).                                                              |
| **Power Switch**     | Mini SPDT Slide/Rocker Switch    | 1   | Rated ≥ 2A.                                                                                                                               |
| **Filtering Caps**   | 0.1 µF (104) Ceramic Cap         | 6   | 1 soldered directly across each N20 motor's terminals to suppress brush noise.                                                            |
| **Bulk Cap**         | 220 µF – 470 µF Electrolytic Cap | 1   | Rated ≥ 10V; placed across motor driver `VM` and `GND` to prevent ESP32 brownouts.                                                        |
| **Protection Diode** | 1N4007 (or 1N5819) Diode         | 1   | Optional inline on solar `+` wire if panel `Voc` exceeds 6.8V in direct sun.                                                              |
| **Hardware & Wire**  | M3 Screws/Nuts & 24 AWG Wire     | —   | Assorted M3 × 8/12/16 mm screws + hex nuts; flexible silicone stranded wire for stems.                                                    |

---

## 3. 3D Printing Guidelines

- **Main Chassis & Rocker-Bogie Stems:** **PLA** or **PETG** (3–4 walls, 20%–30% infill for rigid suspension pivots).
- **Inner Wheel Hubs:** **PETG** (better heat and impact resistance around the motor D-shaft).
- **Outer Tires:** **95A TPU** (2–3 walls, 10%–15% Gyroid infill for grip and bump absorption).

  - *Printer tip:* Load TPU directly into the toolhead (bypassing AMS/MMU feeders) and dry the spool at 50–55°C before printing to prevent stringing.

---

## 4. Electronics & Power Wiring

### A. Power Distribution

1. **Solar → Charger:** Connect the 6V solar panel (`+` / `-`) to the **CN3065** `SOLAR IN` terminals.
2. **Charger → Battery:** Connect the 3.7V Li-ion/LiPo cell to the CN3065 `BAT+` and `BAT-` terminals.
3. **Battery → Boost Converter (Bypass Charger** `LOAD`**):** Wire `BAT+` through the main **power switch** directly into the **5V Boost Converter** `VIN+` (do *not* draw motor current through the CN3065's `LOAD` pin).
4. **Boost Output → System:** Feed the regulated **5.2V–5.5V** output to:

   - Motor driver `VM` (motor power) + **220–470 µF capacitor** across `VM` and `GND`.
   - ESP32 `5V` / `VIN` pin.
5. **Common Ground:** Tie all `GND` lines together (Battery `-`, CN3065 `GND`, Booster `GND`, Motor Driver `GND`, ESP32 `GND`, and Sensors `GND`).

### B. Motor Grouping (Skid Steering)

- **Left Channel (**`AO1` **/** `AO2`**):** Wire all **3 left-side N20 motors in parallel** (ensure identical polarity so all three spin forward together).
- **Right Channel (**`BO1` **/** `BO2`**):** Wire all **3 right-side N20 motors in parallel**.
- **Logic Pins:**
  - *TB6612FNG:* Connect `VCC` and `STBY` to ESP32 **3.3V**; connect `AIN1`, `AIN2`, `PWMA`, `BIN1`, `BIN2`, `PWMB` to 6 PWM-capable ESP32 GPIOs.
  - *DRV8833:* Connect `SLP` to ESP32 **3.3V**; connect `AIN1`, `AIN2`, `BIN1`, `BIN2` to 4 PWM-capable ESP32 GPIOs.

### C. Dual BH1750 I2C Wiring

- Mount one sensor on the front-left corner (~35° outward) and one on the front-right corner (~35° outward).
- Share `VCC` (**3.3V**), `GND`, `SDA` (e.g., `GPIO 21`), and `SCL` (e.g., `GPIO 22`) across both sensors.
- **Set Distinct I2C Addresses:**
  - **Left Sensor:** Leave `ADDR` floating or tie to `GND` → Address `0x23`.
  - **Right Sensor:** Tie `ADDR` to **3.3V** → Address `0x5C`.

---

## 5. Mechanical Assembly Order

1. **Prep Motors:** Solder 0.1 µF ceramic capacitors across each motor's tabs and attach ~20 cm silicone wire leads.
2. **Install Motors:** Slide each N20 motor into its 3D-printed bogie/chassis mount, route wires through the hollow suspension stems, and snap on the printed motor covers.
3. **Assemble Rocker-Bogie Arms:** Join the L-shaped main rocker frame and C-shaped bogie frame using the printed stopper and **M3 bolts/nuts** (snug enough to prevent wobble, loose enough to pivot freely).
4. **Mount Chassis & Wheels:** Bolt left and right suspension assemblies to the central body, feed motor leads into the electronics bay, and press the printed wheel/tire assemblies onto the 3 mm D-shafts.
5. **Mount Top Deck:** Route solar panel wires through the top cover and secure the 6V solar panel flat on top.

---

## 6. Autonomous Sun-Seeking & Charging Logic

1. **Monitor Battery Voltage:** Use a 100 kΩ + 100 kΩ voltage divider from `BAT+` to an **ADC1** pin on the ESP32 (`GPIO 32–39`) to track charge state.
2. **Drive / Explore Mode (**`V_bat > 3.6V`**):** Accept remote commands over ESP-NOW / Wi-Fi or run obstacle/light exploration.
3. **Sun-Seeking Mode (**`V_bat < 3.5V`**):**
   - Read `lux_left` (`0x23`) and `lux_right` (`0x5C`).
   - Turn toward the higher reading until `|lux_left - lux_right|` is within a small deadband (~50–100 lux), then drive forward.
   - Stop once brightness plateaus above a target threshold (e.g., `> 15,000 lux` outdoors or peak indoor light).
4. **Deep Sleep Charge Mode:** Pull motor driver `STBY`/`SLP` **LOW** and put the ESP32 into **deep sleep** (waking every 5–10 minutes to check battery voltage) so active Wi-Fi current doesn't drain the solar charging current.
