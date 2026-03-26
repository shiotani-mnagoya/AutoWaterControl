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

- `src/main.cpp` posts JSON every 10 seconds via `HTTPClient` to `SERVER_URL`.
- Payload currently includes `user_id`, `action`, and `count`.
- Touch edge detection increments `count` only on press transition.
- Wi-Fi SSID/password and endpoint URL are loaded from `include/secrets.h`.
- Secrets are already separated from source and should remain out of git.
- Date-based reset from server response is not implemented yet.

## Migration Delta

- Keep touch counting and periodic POST behavior.
- Change server target from former Google endpoint workflow to FastAPI endpoint.
- Parse FastAPI response JSON and read `server_date`.
- Track last received date and reset `count = 0` when date changes.

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
- Reset logic should only run when response is valid and `server_date` is present.
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
