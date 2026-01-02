# MQTT Standard - Camper Ecosystem

**Version**: 1.0
**Last Updated**: 2026-01-02
**Applies to**: All devices in camper ecosystem (ESP32, Arduino, Raspberry Pi, etc.)

---

## Overview

This document defines the **mandatory MQTT communication standard** for all devices in the camper ecosystem. Consistent MQTT patterns enable reliable data collection, remote control, and system monitoring.

## Broker Configuration

### Primary Broker
- **Host**: `mqtt.syquens.com`
- **Secure Port**: 8883 (TLS/SSL)
- **Insecure Port**: 1883 (unencrypted, local network only)
- **Protocol**: MQTT v3.1.1 (minimum), v5.0 (preferred)

### Authentication
- **Username**: Device-specific or shared service account
- **Password**: Stored securely (NVS on ESP32, environment variables on Linux)
- **Client Certificates**: Optional for enhanced security

### Connection Parameters
```c
// ESP32 example (esp_mqtt_client_config_t)
mqtt_cfg.broker.address.uri = "mqtts://mqtt.syquens.com:8883";
mqtt_cfg.credentials.username = "camper_device";
mqtt_cfg.credentials.client_id = "lindi_AB12CD";  // Unique per device
mqtt_cfg.session.keepalive = 60;  // seconds
mqtt_cfg.network.reconnect_timeout_ms = 5000;
mqtt_cfg.network.disable_auto_reconnect = false;
```

**Python example** (paho-mqtt):
```python
import paho.mqtt.client as mqtt

client = mqtt.Client(client_id="rpi_battery", protocol=mqtt.MQTTv311)
client.username_pw_set(username="camper_device", password="<password>")
client.tls_set()  # Enable TLS
client.connect("mqtt.syquens.com", 8883, keepalive=60)
client.loop_start()
```

### Reconnection Strategy
- **Auto-reconnect**: Always enabled
- **Backoff**: Exponential backoff starting at 1 second, max 60 seconds
- **Persistent session**: Use `clean_session=false` for QoS 1/2 messages

---

## Topic Structure

### Hierarchical Namespace

All topics follow this pattern:
```
<base>/<device_id>/<data_type>
```

**Components**:
- `<base>`: Root namespace (e.g., `camper`, `lindi`)
- `<device_id>`: Unique device identifier (MAC-based or custom)
- `<data_type>`: Type of data (e.g., `level`, `gps`, `temperature`, `battery`, `status`)

### Examples

| Device | Topic | Purpose |
|--------|-------|---------|
| ESP32 Level Sensor | `camper/lindi_AB12CD/level` | Pitch/roll measurements |
| GPS Module | `camper/gps_tracker/gps` | Location data |
| Battery Monitor | `camper/rpi_battery/battery` | Voltage/current readings |
| Temperature Sensor | `camper/temp_01/temperature` | Ambient temperature |
| Device Status | `camper/lindi_AB12CD/status` | Online/offline, health |

### Control Topics (Commands)

Control topics use `/cmd/` suffix:
```
<base>/<device_id>/cmd/<action>
```

**Examples**:
- `camper/lindi_AB12CD/cmd/calibrate` - Trigger level calibration
- `camper/lights_controller/cmd/set_brightness` - Set LED brightness
- `camper/pump_controller/cmd/enable` - Enable water pump

### Response Topics

Devices respond to commands on `/rsp/` topics:
```
<base>/<device_id>/rsp/<action>
```

**Examples**:
- `camper/lindi_AB12CD/rsp/calibrate` - Calibration result
- `camper/pump_controller/rsp/enable` - Command acknowledgment

---

## Message Formats

### Sensor Data Messages

**Standard JSON format**:
```json
{
  "client_id": "lindi_AB12CD",
  "timestamp_ms": 1735840123456,
  "sensor_type": "level",
  "data": {
    "pitch": 1.234,
    "roll": -0.567
  }
}
```

**Required fields**:
- `client_id` (string): Device identifier
- `timestamp_ms` (number): Milliseconds since Unix epoch (UTC)
- `sensor_type` (string): Type of sensor data
- `data` (object): Sensor-specific measurements

**Platform-specific examples**:

**ESP32 (C with cJSON)**:
```c
cJSON *root = cJSON_CreateObject();
cJSON_AddStringToObject(root, "client_id", "lindi_AB12CD");
cJSON_AddNumberToObject(root, "timestamp_ms", (double)timestamp_ms);
cJSON_AddStringToObject(root, "sensor_type", "level");

cJSON *data = cJSON_CreateObject();
cJSON_AddNumberToObject(data, "pitch", (double)pitch);
cJSON_AddNumberToObject(data, "roll", (double)roll);
cJSON_AddItemToObject(root, "data", data);

char *json_string = cJSON_PrintUnformatted(root);
esp_mqtt_client_publish(client, "camper/lindi_AB12CD/level", json_string, 0, 0, 0);
free(json_string);
cJSON_Delete(root);
```

**Python**:
```python
import json
import time

message = {
    "client_id": "rpi_battery",
    "timestamp_ms": int(time.time() * 1000),
    "sensor_type": "battery",
    "data": {
        "voltage": 12.456,
        "current": 2.345,
        "power": 29.12
    }
}

client.publish("camper/rpi_battery/battery", json.dumps(message), qos=0)
```

**JavaScript/Node.js**:
```javascript
const mqtt = require('mqtt');
const client = mqtt.connect('mqtts://mqtt.syquens.com:8883', {
  username: 'camper_device',
  clientId: 'node_temp_sensor'
});

const message = {
  client_id: 'node_temp_sensor',
  timestamp_ms: Date.now(),
  sensor_type: 'temperature',
  data: {
    temperature_c: 23.5,
    humidity_pct: 65.2
  }
};

client.publish('camper/node_temp/temperature', JSON.stringify(message), { qos: 0 });
```

### Status Messages (Last Will & Testament)

**Online status** (published on connect):
```json
{
  "client_id": "lindi_AB12CD",
  "timestamp_ms": 1735840123456,
  "status": "online",
  "firmware_version": "1.0.0",
  "ip_address": "192.168.1.42"
}
```

**Offline status** (LWT, published by broker on disconnect):
```json
{
  "client_id": "lindi_AB12CD",
  "timestamp_ms": 1735840123456,
  "status": "offline"
}
```

**Configure LWT (ESP32)**:
```c
mqtt_cfg.session.last_will.topic = "camper/lindi_AB12CD/status";
mqtt_cfg.session.last_will.msg = "{\"client_id\":\"lindi_AB12CD\",\"status\":\"offline\"}";
mqtt_cfg.session.last_will.msg_len = strlen(mqtt_cfg.session.last_will.msg);
mqtt_cfg.session.last_will.qos = 1;
mqtt_cfg.session.last_will.retain = true;
```

**Configure LWT (Python)**:
```python
offline_message = json.dumps({"client_id": "rpi_battery", "status": "offline"})
client.will_set("camper/rpi_battery/status", offline_message, qos=1, retain=True)
```

### Command Messages

**Command format**:
```json
{
  "command": "calibrate",
  "parameters": {
    "save_to_nvs": true
  },
  "timestamp_ms": 1735840123456
}
```

**Response format**:
```json
{
  "command": "calibrate",
  "status": "success",
  "result": {
    "pitch_offset": 0.123,
    "roll_offset": -0.045
  },
  "timestamp_ms": 1735840125678
}
```

---

## Quality of Service (QoS) Levels

### QoS 0 - At Most Once (Fire and Forget)
**Use for**:
- High-frequency sensor data (level, GPS, temperature)
- Data where occasional loss is acceptable
- Bandwidth-constrained scenarios

**Example**: Level sensor publishing pitch/roll every second

### QoS 1 - At Least Once (Acknowledged)
**Use for**:
- Status messages (online/offline)
- Command acknowledgments
- Important but not critical data

**Example**: Device status updates, command responses

### QoS 2 - Exactly Once (Assured Delivery)
**Use for**:
- Critical commands (e.g., emergency stop)
- Financial transactions
- Non-idempotent operations

**Example**: Water pump enable/disable commands

### Recommendation by Message Type

| Message Type | QoS | Retain | Reason |
|--------------|-----|--------|--------|
| Sensor data (high-freq) | 0 | No | Occasional loss acceptable, next reading coming soon |
| Status (online/offline) | 1 | Yes | Important to know device state, LWT reliability |
| Commands | 1 | No | Must be acknowledged, no need to persist |
| Critical commands | 2 | No | Must execute exactly once |
| Configuration updates | 1 | Yes | Last value needed for late subscribers |

---

## Retained Messages

### When to Use Retained Messages

**Use retained messages for**:
- Device status (online/offline)
- Last known configuration
- Current setpoints (e.g., thermostat temperature)

**Do NOT retain**:
- High-frequency sensor data (creates stale data problem)
- Time-sensitive commands

### Example: Device Status with Retention

**ESP32**:
```c
// Publish online status with retain
char *online_msg = "{\"client_id\":\"lindi_AB12CD\",\"status\":\"online\"}";
esp_mqtt_client_publish(client, "camper/lindi_AB12CD/status", online_msg, 0, 1, 1);
//                                                                          QoS^ ^Retain
```

**Python**:
```python
# Publish online status with retain
online_msg = json.dumps({"client_id": "rpi_battery", "status": "online"})
client.publish("camper/rpi_battery/status", online_msg, qos=1, retain=True)
```

---

## Subscribing to Topics

### Wildcard Patterns

**Single-level wildcard** (`+`):
```
camper/+/level          # All devices publishing level data
camper/lindi_AB12CD/+   # All data types from one device
```

**Multi-level wildcard** (`#`):
```
camper/#                # All topics under camper namespace
camper/lindi_AB12CD/#   # All topics for one device (data + commands + responses)
```

### Subscription Examples

**Monitor all sensor data** (Python):
```python
def on_message(client, userdata, msg):
    data = json.loads(msg.payload)
    print(f"Topic: {msg.topic}")
    print(f"Device: {data['client_id']}")
    print(f"Sensor: {data['sensor_type']}")
    print(f"Data: {data['data']}")

client.on_message = on_message
client.subscribe("camper/#", qos=0)  # Subscribe to all camper topics
client.loop_forever()
```

**Control specific device** (ESP32):
```c
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    if (event->event_id == MQTT_EVENT_DATA) {
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

        // Parse and handle command
        cJSON *root = cJSON_Parse(event->data);
        const char *command = cJSON_GetObjectItem(root, "command")->valuestring;

        if (strcmp(command, "calibrate") == 0) {
            // Execute calibration
            perform_calibration();
        }

        cJSON_Delete(root);
    }
}

// Subscribe to commands
esp_mqtt_client_subscribe(client, "camper/lindi_AB12CD/cmd/#", 1);
```

---

## Data Rates & Bandwidth

### Recommended Update Intervals

| Sensor Type | Update Interval | Topic QoS | Reason |
|-------------|-----------------|-----------|--------|
| Level (pitch/roll) | 1 Hz (1000ms) | 0 | Balance accuracy vs bandwidth |
| GPS | 1 Hz (1000ms) | 0 | Standard GPS update rate |
| Temperature | 0.1 Hz (10s) | 0 | Slow-changing data |
| Battery voltage | 0.2 Hz (5s) | 0 | Moderate monitoring |
| Accelerometer | 10 Hz (100ms) | 0 | Motion detection (use carefully) |
| Status | On change only | 1 | Event-driven |

### Bandwidth Estimation

**Example message** (level data):
```json
{"client_id":"lindi_AB12CD","timestamp_ms":1735840123456,"sensor_type":"level","data":{"pitch":1.234,"roll":-0.567}}
```
- **Message size**: ~130 bytes
- **Update rate**: 1 Hz
- **Bandwidth**: 130 bytes/s = 1.04 kbit/s

**10 devices** @ 1 Hz: ~10 kbit/s = **minimal bandwidth usage**

---

## Security Best Practices

### TLS/SSL Encryption
- **Always use port 8883** (TLS) for internet-connected brokers
- **Use port 1883** (unencrypted) only for local network testing
- **Verify server certificate** to prevent man-in-the-middle attacks

### Authentication
- **Unique credentials per device** (preferred) or shared service account
- **Store credentials securely**: NVS on ESP32, environment variables on Linux
- **Never hardcode credentials** in source code

### Authorization (ACL)
Configure broker to restrict topic access per client:
- Device `lindi_AB12CD` can only publish to `camper/lindi_AB12CD/#`
- Monitoring service can subscribe to `camper/#` but not publish
- Control service can publish to `camper/+/cmd/#` but not sensor topics

---

## Error Handling & Reliability

### Connection Loss Handling

**ESP32 example**:
```c
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            // Resubscribe to topics
            esp_mqtt_client_subscribe(client, "camper/lindi_AB12CD/cmd/#", 1);

            // Publish online status
            char *online_msg = "{\"client_id\":\"lindi_AB12CD\",\"status\":\"online\"}";
            esp_mqtt_client_publish(client, "camper/lindi_AB12CD/status", online_msg, 0, 1, 1);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            // Auto-reconnect handled by esp_mqtt library
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error: %d", event->error_handle->error_type);
            break;
    }
}
```

**Python example**:
```python
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker")
        # Resubscribe (in case of reconnection)
        client.subscribe("camper/rpi_battery/cmd/#", qos=1)
        # Publish online status
        online_msg = json.dumps({"client_id": "rpi_battery", "status": "online"})
        client.publish("camper/rpi_battery/status", online_msg, qos=1, retain=True)
    else:
        print(f"Connection failed with code {rc}")

def on_disconnect(client, userdata, rc):
    if rc != 0:
        print("Unexpected disconnect, reconnecting...")

client.on_connect = on_connect
client.on_disconnect = on_disconnect
```

### Message Queuing During Offline

**Persistent session** (QoS 1/2):
```c
mqtt_cfg.session.disable_clean_session = true;  // Keep session state
```

**Local buffering** (QoS 0):
- Store messages locally during offline periods
- Publish when reconnected (with original timestamp)
- Limit buffer size to prevent memory overflow

---

## Testing & Debugging

### Command-Line Tools

**Subscribe to all topics** (mosquitto_sub):
```bash
mosquitto_sub -h mqtt.syquens.com -p 8883 --cafile /etc/ssl/certs/ca-certificates.crt \
  -u camper_device -P <password> -t 'camper/#' -v
```

**Publish test message** (mosquitto_pub):
```bash
mosquitto_pub -h mqtt.syquens.com -p 8883 --cafile /etc/ssl/certs/ca-certificates.crt \
  -u camper_device -P <password> -t 'camper/test/level' \
  -m '{"client_id":"test","timestamp_ms":1735840123456,"sensor_type":"level","data":{"pitch":0,"roll":0}}'
```

### Python Test Script

```python
import paho.mqtt.client as mqtt
import json
import time

def on_message(client, userdata, msg):
    print(f"\n[{msg.topic}]")
    try:
        data = json.loads(msg.payload)
        print(json.dumps(data, indent=2))
    except:
        print(msg.payload.decode())

client = mqtt.Client(client_id="test_monitor")
client.username_pw_set("camper_device", "<password>")
client.tls_set()
client.on_message = on_message

client.connect("mqtt.syquens.com", 8883)
client.subscribe("camper/#", qos=0)

print("Monitoring all camper topics (Ctrl+C to stop)...")
client.loop_forever()
```

---

## Implementation Checklist

When adding MQTT to a new device:

- [ ] Include MQTT client library (esp_mqtt, paho-mqtt, etc.)
- [ ] Configure broker URI with TLS (port 8883)
- [ ] Set unique client ID (MAC-based or custom)
- [ ] Configure username/password authentication
- [ ] Set Last Will & Testament (offline status)
- [ ] Implement connection event handler
- [ ] Publish online status on connect (QoS 1, retain)
- [ ] Subscribe to command topics (if needed)
- [ ] Use standard JSON message format with `timestamp_ms`
- [ ] Choose appropriate QoS for each message type
- [ ] Handle reconnection gracefully
- [ ] Test with mosquitto_sub before deployment
- [ ] Document device topic structure
- [ ] Add to ecosystem device inventory

---

## Ecosystem Device Inventory

### Current Devices

| Device ID | Type | Topics Published | Topics Subscribed | Update Rate |
|-----------|------|------------------|-------------------|-------------|
| lindi_AB12CD | ESP32 Level Sensor | `camper/lindi_AB12CD/level` | `camper/lindi_AB12CD/cmd/#` | 1 Hz |
| lindi_AB12CD | ESP32 Level Sensor | `camper/lindi_AB12CD/status` | - | On change |

### Planned Devices

| Device ID | Type | Topics Published | Topics Subscribed | Update Rate |
|-----------|------|------------------|-------------------|-------------|
| gps_tracker | ESP32 GPS | `camper/gps_tracker/gps` | `camper/gps_tracker/cmd/#` | 1 Hz |
| rpi_battery | Raspberry Pi | `camper/rpi_battery/battery` | `camper/rpi_battery/cmd/#` | 0.2 Hz |
| temp_01 | Arduino DS18B20 | `camper/temp_01/temperature` | - | 0.1 Hz |

---

## Related Ecosystem Standards

See also:
- [timestamp_ecosystem.md](timestamp_ecosystem.md) - Standardized timestamp format
- [serial_menu_ecosystem.md](serial_menu_ecosystem.md) - Serial configuration interface
- [nvs_config_ecosystem.md](nvs_config_ecosystem.md) - Configuration storage patterns

---

**Ecosystem Owner**: Syquens B.V.
**Maintained by**: V.N. Verbon
**Version Control**: Git repository `Lindi/export/`
