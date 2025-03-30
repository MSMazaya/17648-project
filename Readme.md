# Vehicle API – VAPI

Name: Muhammad Mazaya

Andrew ID: mmazaya

---

## Usage

### Build and Run

To use it in a terminal-style interaction:

    ./run.sh

This launches the container and allows direct input via stdin:

    GET oil_temp
    GET fuel_level_liters
    GET ALL
    START PUSH
    STOP

---

## Repository Structure

    .
    ├── Dockerfile          # Builds Ubuntu-based container and compiles VAPI
    ├── run.sh              # Builds the image and runs either container or CLI
    ├── user_prompt.py      # Python client for interactive user input
    ├── vapi.c              # Main C source file (Vehicle API implementation)
    └── README.md           # This documentation

---

## Design Considerations

- Push and Pull Support  
  The Vehicle API supports both pull mode (single field access via `GET <field>`) and push mode (`START PUSH`) for continuous periodic data streaming.

- Serial Communication Stub Using STDOUT/STDIN

  Communication is handled via standard input and output streams. This is mimicking serial communication input and output stream like UART.

  The simulated data is represented as a packed C struct to reflect realistic embedded system layouts which can be queried using the `GET` command with the following list of fields.

    - `oil_temp`  
    - `maf_raw`  
    - `maf_cfm`  
    - `battery_voltage`  
    - `tire_pressure_raw`  
    - `tire_pressure_psi`  
    - `fuel_level_liters`  
    - `fuel_consumption_rate`  
    - `error_codes`

  A special `GET ALL` command returns a full JSON-formatted snapshot of all fields.


- Sensor conversions

  Raw ADC sensor data is simulated and linearly converted to physical units using system-provided scaling factors (as specified in the project descriptoin):

    - **Tire Pressure (psi)**  
      Raw range: 0–2047 (11-bit)  
      Physical range: 0–100 psi  
      Formula: `tire_pressure_psi = raw_value × (100 / 2047)`

    - **Mass Air Flow (cfm)**  
      Raw range: 0–2047 (11-bit)  
      Physical range: 0–2500 cfm  
      Formula: `maf_cfm = raw_value × (2500 / 2047)`

  Other fields like oil temperature, fuel level, battery voltage, and error codes are reported directly as simulated values within expected operational ranges.
