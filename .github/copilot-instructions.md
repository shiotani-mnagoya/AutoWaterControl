# Copilot Workspace Instructions

このリポジトリでは、ESP32からFastAPIへデータ送信し、CSVへ記録する移行作業を優先します。

## Skill Loading

FastAPI移行関連の作業を行うときは、次のスキルを参照してから実装してください。

- [FastAPI CSV Migration Skill](./skills/fastapi-csv-migration/SKILL.md)

## Working Rules

- `include/secrets.h` は機密情報を含むため、作成は許可するがコミットしない。
- `include/secrets.example.h` はテンプレートとして維持する。
- ESP32側はサーバ応答の `server_date` を使い、日付変更時に `count` を0へリセットする。
- サーバ側は受信JSONをCSVへ追記し、レスポンスで時刻情報を返す。
