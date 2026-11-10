# MiniFab

MiniFab is a **C++20 semiconductor equipment-control system** operating simulated hardware. It implements equipment state management, wafer tracking, recipe execution, asynchronous process orchestration, cancellation, fault recovery, persistent events, a REST API, and a browser-based operator interface.

The hardware is simulated; the equipment-control logic, API, persistence, and tests are real software components.

## Current capabilities

- Simulated robot, process module, and wafer movement/tracking
- Equipment state machine and asynchronous process execution
- Recipe management backed by SQLite
- Run, Cancel, and Reset operations through the controller and REST API
- Equipment snapshots and event history exposed through REST endpoints
- SQLite-backed event persistence and run correlation
- Browser pages for running equipment and managing recipes
- JavaScript model/ViewModel separation, status polling, and event updates
- C++ and JavaScript automated tests
- CMake deployment of browser assets alongside the web executable

Reset is intended for fault recovery, not routine idle operation. Available UI commands depend on equipment state.

## Architecture

| Directory | Responsibility |
| --- | --- |
| `domain/`, `wafer/` | Core domain types and wafer tracking |
| `equipment/` | Simulated hardware implementations |
| `engine/` | Process orchestration, concurrency, and state transitions |
| `controller/` | Application operations and coordination |
| `recipes/` | Recipe management |
| `persistence/` | SQLite repositories |
| `web/routes/` | REST API endpoints |
| `web/parsing/`, `web/serialization/` | API input and output translation |
| `web/public/` | HTML, CSS, and JavaScript operator interface |
| `tests/` | C++ and JavaScript tests |
| `app/` | Executable entry points |

The browser UI uses a lightweight model/ViewModel structure. The C++ controller and engine remain authoritative for equipment behavior and state.

## Prerequisites

- C++20-capable compiler
- CMake 3.20 or newer
- Git and network access for CMake's initial dependency fetch
- Node.js and npm for JavaScript tests

CMake fetches GoogleTest, SQLiteCpp, and cpp-httplib as configured in `CMakeLists.txt`.

## Build and test

Build and test the project using the existing script:

```bash
./build.sh
```

To include the web server:

```bash
BUILD_WEB=ON ./build.sh
```

Or configure and build manually:

```bash
cmake -S . -B build -DMINIFAB_BUILD_WEB=ON
cmake --build build -j2
ctest --test-dir build --output-on-failure
npm test
```

The web target is optional and is **OFF by default** in CMake. The build script runs both C++ and JavaScript tests; Node.js and npm are required for the latter.

## Run the web interface

After building with `MINIFAB_BUILD_WEB=ON`, start the executable from the project root:

```bash
./build/mini_fab_web
```

The server currently listens on `127.0.0.1:8080`. Open:

- `http://127.0.0.1:8080/run.html` — equipment operation and event display
- `http://127.0.0.1:8080/recipes.html` — recipe management
- `http://127.0.0.1:8080/wafers.html` — wafer inventory and registration

CMake copies `web/public/` to the web executable's `public/` directory **on every build**, including when only frontend files changed. The executable mounts that build-output directory, rather than relying on a source-tree-relative working directory.

After editing browser assets, **rebuild and restart as needed**. During development, stale browser JavaScript caches can cause the UI to execute an older module even when tests and source code are correct; use a fresh/private session or clear the site's cache to diagnose this.

## API overview

Current routes include:

- `POST /api/run` — request a recipe run
- `POST /api/cancel` — request cancellation
- `POST /api/reset` — request fault recovery
- `GET /api/status` — read equipment status
- `GET /api/events` — read persisted events
- Recipe endpoints under `/api/recipes`
- `GET /api/wafers` — list registered wafers
- `POST /api/wafers` — register a wafer

Commands are subject to equipment-state and controller validation. The UI should reflect server-confirmed outcomes, not assume a command succeeded because it was clicked.

## Wafer registration API

### List wafers

`GET /api/wafers` returns HTTP `200` with a JSON object containing a `wafers` array. For example:

```json
{
  "wafers": [
    {
      "id": "W1",
      "location": {
        "station": "LoadPort",
        "slot": 3
      }
    }
  ]
}
```

When the inventory is empty, the response is `{"wafers":[]}`.

### Register a wafer

`POST /api/wafers` requires the `Content-Type: application/json` header and a JSON object with these fields:

| Field | Type | Description |
| --- | --- | --- |
| `id` | string | Nonempty wafer identifier, unique within the inventory |
| `station` | string | Must be `"LoadPort"` for initial registration |
| `slot` | integer | Positive LoadPort slot number; the slot must be unoccupied |

Example request:

```http
POST /api/wafers
Content-Type: application/json

{"id":"W2","station":"LoadPort","slot":4}
```

Successful registration returns HTTP `201 Created` with `Content-Type: application/json` and the registered wafer:

```json
{
  "id": "W2",
  "location": {
    "station": "LoadPort",
    "slot": 4
  }
}
```

A subsequent `GET /api/wafers` includes the newly registered wafer.

### Error responses

Errors are JSON objects with an `error` string. For example:

```json
{"error":"Wafer ID already exists"}
```

| HTTP status | When returned |
| --- | --- |
| `400 Bad Request` | Malformed JSON, missing fields, wrong field types, unknown or duplicate fields, integer overflow, or trailing content |
| `409 Conflict` | Duplicate wafer ID or occupied LoadPort slot |
| `415 Unsupported Media Type` | Request does not use an accepted `application/json` Content-Type |
| `422 Unprocessable Content` | Semantically invalid registration, such as an empty wafer ID, nonpositive slot, or station other than `LoadPort` |

Conflicting registrations must leave the existing inventory unchanged. If concurrent requests attempt to register the same wafer ID or occupy the same slot, at most one may succeed.

The browser registration interface is available at `/wafers.html`.

## Wafer location rules (v0.9)

MiniFab currently models a single Aligner and a single-wafer ProcessModule. A `Location` consists of a station and a slot; the slot identifies a wafer position, **not** a process-module ID.

| Station | Valid slot numbers | Occupancy rule |
| --- | --- | --- |
| `LoadPort` | Positive integers (1+) | One wafer per slot |
| `Aligner` | 1 | One wafer total |
| `ProcessModule` | 1 | One wafer total |
| `Output` | Positive integers (1+) | One wafer per slot |

New wafers may only be registered at the LoadPort. `WaferManager::updateLocation` rejects unknown wafer IDs, invalid stations/slots, and occupied destinations. It validates and updates under the manager mutex, so competing moves cannot claim the same destination. Rejected moves leave the in-memory location unchanged; successful moves are persisted when a repository is configured. Updating a wafer to its existing location succeeds without changing it.

These rules validate **destination validity and occupancy**, not the full physical movement path. The process engine coordinates movement, processing, cancellation, and recovery. Multi-module equipment and configurable chamber capacity are future extensions; do not interpret `ProcessModule, slot 2` as a second chamber.

## Data and persistence

The application uses `minifab.db` as its local SQLite database, relative to the process working directory. Recipe and event data can survive process restarts.

**Runtime databases are not source code.** `.gitignore` excludes `minifab.db` and its SQLite sidecar files. Do not commit production or local runtime data. Removing a tracked database from the index does not erase historical commits.

## Development principles

See [CONTRIBUTING.md](CONTRIBUTING.md) for coding, architecture, testing, and review guidelines.

MiniFab is developed incrementally: implement one well-defined behavior, verify it at the relevant layers, run the full tests, then commit. Future ideas such as fault injection or event-log clearing are not implemented merely because they appear on a backlog.
