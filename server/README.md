# FastAPI Server

ESP32から受け取ったJSONをCSVへ追記するローカルサーバです。

## Setup (Windows PowerShell)

```powershell
cd server
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
uvicorn app.main:app --host 0.0.0.0 --port 8000 --reload
```

## Endpoints

- `GET /health`
- `POST /ingest`

Request example:

```json
{
  "user_id": "esp32",
  "action": "click",
  "count": 3
}
```

Response example:

```json
{
  "ok": true,
  "server_datetime": "2026-03-26T10:15:30+09:00",
  "server_date": "2026-03-26"
}
```
