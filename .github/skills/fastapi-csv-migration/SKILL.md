---
name: fastapi-csv-migration
description: "Use when: migrating ESP32 telemetry posting from Google Apps Script to FastAPI on Windows, logging received JSON data into CSV, and resetting count on ESP32 when date changes from FastAPI time. Keywords: esp32, fastapi, csv, server, date reset, count reset, wifi, http post"
---

# FastAPI CSV Migration Skill

## Purpose

Migrate an ESP32 data pipeline from Google endpoint posting to a local FastAPI server on Windows.

## Goals

- Replace Google endpoint usage with FastAPI endpoint.
- Save incoming JSON payloads to CSV on the server.
- Return server date/time from FastAPI response.
- Update ESP32 logic to reset `count` when the server date changes.

## Current State (main.cpp)

- `src/main.cpp` has two responsibilities running in parallel:
  - `loop()` handles touch sensing, edge detection, and OUTPUT pin control.
  - `wifiTask` (pinned to Core1) posts JSON to `SERVER_URL` every 10 seconds.
- Wi-Fi connection start is non-blocking in `setup()` (no wait loop), so touch detection and output control run even when Wi-Fi is not connected.
- Touch behavior uses `touchRead(T0)` and a threshold (`value < 50`) to detect press.
- `count` is incremented only on press transition (`isTouched && !wasTouched`) to avoid repeat increments during continuous touch.
- This means count increases when touch starts (edge), not while a finger is continuously held on the sensor.
- Touch state is reflected to hardware output with `digitalWrite(OUTPUT_PIN, isTouched ? HIGH : LOW)`.
- `wasTouched = isTouched` updates previous state each loop to keep edge detection stable.
- Wi-Fi credentials and endpoint URL are loaded from `include/secrets.h`.
- Posted payload includes `user_id`, `action`, and `count`.
- When Wi-Fi is disconnected, `wifiTask` attempts `WiFi.reconnect()` and skips POST until connected.
- Server response is parsed with ArduinoJson, and `server_date` is used for daily reset logic.
- If `server_date` changes from the previous valid value, `count` is reset to `0` and `lastServerDate` is updated.
- Secrets are already separated from source and should remain out of git.

## Migration Delta

- Keep touch counting and periodic POST behavior.
- Change server target from former Google endpoint workflow to FastAPI endpoint.
- Keep FastAPI response parsing for `server_date`.
- Keep date-change reset behavior (`count = 0`) based on server date.

## Inputs To Confirm

- FastAPI server host and port on Windows (example: `192.168.1.10:8000`).
- Endpoint path for ingest (example: `/ingest`).
- Time format returned by API (recommended: ISO 8601, local time with timezone).
- CSV file path and columns.

## Implementation Plan

1. Create FastAPI app with POST endpoint.
2. Validate JSON fields (`user_id`, `action`, `count`, optional `device_time`).
3. Append records to CSV file with header auto-creation.
4. Include server timestamp and server date in response JSON.
5. Update ESP32 `serverUrl` to FastAPI endpoint.
6. Parse response body on ESP32 and track last server date.
7. Reset `count = 0` when server date differs from stored date.
8. Keep Wi-Fi secrets in ignored local header (`include/secrets.h`).

## Recommended API Contract

Request JSON example:

```json
{
  "user_id": "esp32",
  "action": "click",
  "count": 5
}
```

Response JSON example:

```json
{
  "ok": true,
  "server_datetime": "2026-03-26T10:15:30+09:00",
  "server_date": "2026-03-26"
}
```

## CSV Columns

Recommended column order:

- `received_at`
- `server_date`
- `user_id`
- `action`
- `count`
- `client_ip`

## ESP32 Notes

- Keep HTTP timeout and response-code checks.
- Reset logic runs only when response JSON is valid and `server_date` is present.
- Preserve edge-detection touch counting behavior.

## Security Notes

- Do not commit secrets in source files.
- Keep `include/secrets.h` in `.gitignore`.
- Commit only `include/secrets.example.h` template.

## Done Criteria

- ESP32 posts successfully to FastAPI.
- CSV rows are appended per request.
- `count` resets to zero exactly when date changes on server.
- No credentials are tracked by git.
