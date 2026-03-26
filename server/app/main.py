from __future__ import annotations

import csv
from datetime import datetime
from pathlib import Path

from fastapi import FastAPI, Request
from pydantic import BaseModel, Field

app = FastAPI(title="AutoWaterControl API")

BASE_DIR = Path(__file__).resolve().parents[1]
CSV_PATH = BASE_DIR / "data" / "events.csv"
CSV_HEADERS = [
    "received_at",
    "server_date",
    "user_id",
    "action",
    "count",
    "client_ip",
]


class IngestPayload(BaseModel):
    user_id: str
    action: str
    count: int = Field(ge=0)


def append_csv_row(row: dict[str, str | int]) -> None:
    CSV_PATH.parent.mkdir(parents=True, exist_ok=True)
    write_header = not CSV_PATH.exists()

    with CSV_PATH.open("a", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=CSV_HEADERS)
        if write_header:
            writer.writeheader()
        writer.writerow(row)


@app.get("/health")
def health() -> dict[str, bool]:
    return {"ok": True}


@app.post("/ingest")
def ingest(payload: IngestPayload, request: Request) -> dict[str, str | bool]:
    now = datetime.now().astimezone()
    server_datetime = now.isoformat(timespec="seconds")
    server_date = now.date().isoformat()
    client_ip = request.client.host if request.client else ""

    append_csv_row(
        {
            "received_at": server_datetime,
            "server_date": server_date,
            "user_id": payload.user_id,
            "action": payload.action,
            "count": payload.count,
            "client_ip": client_ip,
        }
    )

    return {
        "ok": True,
        "server_datetime": server_datetime,
        "server_date": server_date,
    }
