# ESP32-HTTP-JSON-API-CLIENT — fetch JSON from a public HTTPS API and parse the fields with ArduinoJson

IoT example by [Fluidwire](https://fluidwire.com/).

The ESP32 polls an HTTPS JSON API once a minute, validates the server certificate against a root CA you provide, and extracts only the fields it needs using an ArduinoJson filter and a streaming parse.

## What it does

- Connects to WiFi using credentials in `config.h`
- Sends an HTTPS GET to the API (default: Open-Meteo current weather for Manila, free, no key)
- Verifies the server with a root CA (`WiFiClientSecure::setCACert`)
- Parses the response as a stream with an ArduinoJson filter, so only temperature, humidity and wind are kept in RAM
- Handles WiFi drops, HTTP errors, timeouts and bad JSON without crashing

## Required hardware

- 1x ESP32 dev board (DevKitC, NodeMCU-32, WROOM — any WiFi-capable ESP32)
- No external parts needed

## Required libraries

- **ArduinoJson** by Benoit Blanchon (v7+) — install via **Sketch → Include Library → Manage Libraries**
- `WiFi.h`, `HTTPClient.h` and `WiFiClientSecure.h` ship with the **ESP32 Arduino core** (Boards Manager → `esp32` by Espressif Systems)

## Setup

1. Copy `config.h.example` to `config.h`.
2. Set your WiFi SSID/password. Optionally change the latitude/longitude in `API_URL`.
3. Paste the API's root CA into `API_ROOT_CA` (see below).
4. Select **Tools → Board → ESP32 Dev Module**, pick the COM port, click **Upload**.

### Getting the root CA

```
openssl s_client -showcerts -connect api.open-meteo.com:443 </dev/null
```

Copy the **last** certificate in the chain (the root, or the highest one the server sends) including the `BEGIN`/`END` lines. If the server does not send the root, download it from the CA's website (e.g. the issuer named in the chain). Put each PEM line in quotes with `\n` as in `config.h.example`.

For a 30-second bench test only, set `ALLOW_INSECURE_TLS` to `1`. Never ship that.

## Expected output

Serial Monitor, baud **115200**:

```
WiFi: connecting to your-wifi-ssid ... OK, ip=192.168.1.42
[2026-06-01T10:15] temp=31.4 C  humidity=68 %  wind=9.7 km/h
[2026-06-01T10:30] temp=31.6 C  humidity=67 %  wind=10.1 km/h
```

## Using a different API

1. Change `API_URL` (and `API_HOST`) in `config.h`.
2. Edit the `filter[...]` lines in `fetchAndParse()` to list the keys you want.
3. Edit the `cur["..."]` reads to match.

The filter keeps memory use small even when the API returns a large payload.

## Troubleshooting

- **`HTTP GET failed: connection refused` / `-1`** — TLS handshake failed. The root CA is wrong or incomplete and does not match the server's chain. Re-run the `openssl` command.
- **`HTTP GET -> 429`** — you are polling too fast. Raise `POLL_INTERVAL_MS`.
- **`JSON parse failed: EmptyInput` / `IncompleteInput`** — response was cut off; check signal strength and try again.
- **`JSON: no 'current' object`** — the API changed or returned an error object; open the URL in a browser and compare the shape.
- **WiFi connect spins forever** — wrong SSID/password, or 5 GHz-only network. Classic ESP32 is 2.4 GHz only.

## Security note

Don't commit `config.h`. The `.gitignore` excludes it; only `config.h.example` is tracked. Don't ship `setInsecure()` to production, and keep API keys (if your API needs one) in `config.h`, never in the sketch.

## License

MIT — see [LICENSE](LICENSE).

## Got stuck?

Drop by [fluidwire.com](https://fluidwire.com/) — we ship IoT building blocks and are happy to help.
