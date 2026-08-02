# Vibesense server

JSON HTTP API for the Vibesense detector.

## Run

The server loads the ONNX artifact at startup and listens on `0.0.0.0:5000`:

```sh
cargo run -p vibesense-server --release
```

Configuration is supplied through environment variables:

- `VIBESENSE_MODEL_DIR` — exported classifier directory (default: `detector-onnx`)
- `VIBESENSE_BIND` — complete socket address, such as `127.0.0.1:8080`
- `VIBESENSE_HOST` / `VIBESENSE_PORT` — used when `VIBESENSE_BIND` is absent
- `GITHUB_TOKEN` or `GH_TOKEN` — optional GitHub token, needed for private repositories and strongly recommended for repository scans; GitHub's anonymous 60-request/hour limit is too small for many repositories

`VIBESENSE_BIND` takes precedence over the host and port variables. The model path is relative to the process working directory.

## API

All request and response bodies are JSON. Requests are limited to 2 MiB.

### `POST /code`

Classify plain source code:

```sh
curl http://localhost:5000/code \
  -H 'content-type: application/json' \
  -d '{"code":"fn main() { println!(\"hello\"); }"}'
```

A successful response is a classifier result:

```json
{
  "prediction": "human",
  "ai_probability": 0.18,
  "human_probability": 0.82,
  "chunks": 1,
  "chunk_ai_probabilities": [0.18],
  "aggregation": "mean"
}
```

### `POST /github`

Analyze source files sampled from a GitHub repository:

```sh
curl http://localhost:5000/github \
  -H 'content-type: application/json' \
  -d '{"repository":"owner/repository","ref":"main"}'
```

`repository` accepts `owner/repository` or a GitHub HTTPS, SSH, or scp-style URL. `ref` is optional and may be a branch, tag, or commit SHA. The response is the pipeline's repository statistics, including selection counts, scan counts, confidence interval, and stop reason.

### `POST /github-stream`

Run the same analysis while receiving server-sent events as selection and file scans finish:

```sh
curl -N http://localhost:5000/github-stream \
  -H 'content-type: application/json' \
  -d '{"repository":"owner/repository","ref":"main"}'
```

The endpoint accepts the same request body as `POST /github`. It emits `progress` events containing `RepoStats`, followed by one `complete` event with the final statistics. If an analysis fails after the stream opens, it emits an `error` event using the normal error-body shape. Keep-alive comments are sent every 15 seconds.

### `GET /health`

Returns `{"status":"ok"}`. Because the model is loaded before the listener starts, this also acts as a readiness check.

Errors use a consistent JSON shape:

```json
{
  "error": {
    "code": "invalid_repository",
    "message": "..."
  }
}
```
