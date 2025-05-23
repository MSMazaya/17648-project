# Vehicle API – VAPI

Name: Muhammad Mazaya

Andrew ID: mmazaya

Github URL: https://github.com/MSMazaya/17648-project

---

## Usage

### Build and Run

To run the whole simulation, execute the following command:

    ./run.sh

If running simulation multiple times is needed, one must clean up the docker containers using the following command:

    ./clean.sh

---

## Repository Structure

    .
    ├── clean.sh              # Cleanup script to remove containers and images
    ├── docker-compose.yml    # Launches Gateway + multiple VAPI + Broker containers
    ├── run.sh                # Builds and launches the full simulation via Docker Compose
    ├── gateway/
    │   ├── Dockerfile        # Python-based TCP Gateway container
    │   └── gateway.py        # Gateway server implementation
    ├── vapi/
    │   ├── Dockerfile        # VAPI container (TCP client)
    │   └── vapi.c            # Vehicle API implementation
    ├── broker/
    │   └── config/
    │       ├── mosquitto.conf     # Mosquitto configuration file
    │       └── pwfile             # Password file for authentication (not used, required)
    └── Readme.md             

---

## Design Considerations

  - TCP/IP Communication

    The VAPI has been refactored from a serial-style stdin/stdout tool into a standalone TCP client. It connects to a centralized Gateway over TCP, sends an identifying message (vehicle_id: <id>), and continuously pushes sensor data every 2 seconds. The Gateway listens for incoming connections and logs the data it receives, associating it with the appropriate vehicle ID.

  - Gateway and Connection Table
  
    The Gateway maintains an in-memory table of all active vehicle connections. Each entry includes the vehicle ID, IP address, and connection status. The table is updated as vehicles connect or disconnect. The Gateway prints connection state changes and incoming messages tagged with the associated ID.

  - Multiple Vehicle API Nodes

    Each VAPI container runs a self-contained sensor simulator. The simulated data is represented as a packed C struct to reflect the Vehicle API data. The data is serialized into JSON before being sent to the Gateway.

- Sensor conversions

  Raw ADC sensor data is simulated and linearly converted to physical units using system-provided scaling factors (as specified in the project description):

    - **Tire Pressure (psi)**  
      Raw range: 0–2047 (11-bit)  
      Physical range: 0–100 psi  
      Formula: `tire_pressure_psi = raw_value × (100 / 2047)`

    - **Mass Air Flow (cfm)**  
      Raw range: 0–2047 (11-bit)  
      Physical range: 0–2500 cfm  
      Formula: `maf_cfm = raw_value × (2500 / 2047)`

  Other fields like oil temperature, fuel level, battery voltage, and error codes are reported directly as simulated values within expected operational ranges.

---

## Gateway

The Gateway is implemented in Python and runs as a standalone container. It listens on TCP port 9010 (configurable via environment variables) and waits for incoming connections from one or more vehicles.

Each VAPI vehicle connects to the Gateway and sends an initial identification message in the form of:

    vehicle_id: truck_1

The Gateway keeps track of all connected vehicles and maintains a connection table. This table includes the vehicle ID, IP address, and current connection status. The Gateway also logs all incoming sensor data, tagging each message with the corresponding vehicle ID.

Example log output:

    [12:00:01] Connected: truck_1 (172.20.0.3)
    [12:00:01] Connected: truck_2 (172.20.0.4)
    [12:00:03] [truck_1] 
    {...}
    [12:00:03] [truck_2] 
    {...}

Vehicle disconnections are also tracked and reflected in the connection table.

---

## MQTT Broker

The system uses Eclipse Mosquitto as the MQTT broker, running as a separate container in `network_mode: host`. It is exposed to the host on port `1883`, allowing external tools or scripts to subscribe to published sensor data.

Example to manually subscribe from host:

    mosquitto_sub -h localhost -t 'vehicles/+/sensor/#' -v

The broker is configured via `mqtt/mosquitto.conf`. Anonymous access is enabled for local testing.

---

## Simulation (Multi-Vehicle)

The system is designed to simulate multiple vehicles using Docker Compose. Each vehicle is implemented as a separate VAPI container. The `docker-compose.yml` sets up three instances:

    - vapi1 ----using env----> VEHICLE_ID=truck_1
    - vapi2 ----using env----> VEHICLE_ID=truck_2
    - vapi3 ----using env----> VEHICLE_ID=truck_3

Each vehicle container reads its `VEHICLE_ID` from the environment and connects to the Gateway using TCP. Once connected, it continuously publishes sensor data every 2 seconds.

Communication between all containers happens over an internal bridge network defined in `docker-compose.yml`. No external port exposure is necessary, and all inter-container traffic is routed through the shared virtual network (`vapi_net`).
