For Testing Changes

# OpenBench Server API

Scripting endpoints for creating tests and reading test/workload state.
Base URL examples use `https://ob.kjljixx.com`

## Authentication

Both endpoints accept credentials as form fields in the POST body:

| Field | Description |
| --- | --- |
| `username` | OpenBench account username |
| `password` | OpenBench account password |

**Test creation** (`/scripts/`) always requires an **enabled** user. The request logs in via these fields before creating the workload.

**Test state** (`/api/workload/...`) uses `api_authenticate`:

- If `require_login_to_view` is `false` in server config (default on this instance), the endpoint is public and credentials are optional.
- If `require_login_to_view` is `true`, you must POST an enabled user's `username` / `password` (or already be logged in via browser session).

CSRF is exempt on both endpoints.

---

## 1. Test Creation

Create an SPRT or fixed-games engine test.

| | |
| --- | --- |
| **URL** | `POST /scripts/` |
| **Content-Type** | `application/x-www-form-urlencoded` (or multipart) |
| **Action** | `action=CREATE_TEST` |

This hits the same creation path as the website form at `/test/new/`.

**Scripted** requests (`POST /scripts/` with `action=CREATE_TEST`) return JSON with the new workload id. The website form still redirects to `/index/`.

### Getting the workload id

On success the response body is:

```json
{
  "workload_id": 1234
}
```

Optional `warning` is included when Dev appears behind Base:

```json
{
  "workload_id": 1234,
  "warning": "Consider Rebasing: Dev (my-feature) appears behind Base (main)"
}
```

Use that id with the [test state](#2-test-state) endpoint and the website page `/test/<workload_id>/`:

```python
r = requests.post(URL, data=data)
r.raise_for_status()
workload_id = r.json()['workload_id']
print(workload_id)  # e.g. 1234

# Poll progress
state = requests.post(f'https://ob.kjljixx.com/api/workload/{workload_id}/results/').json()
```

On failure (auth or validation), the response is JSON with HTTP 401 / 400:

```json
{ "error": "SPRT Bounds must be formatted as [float1, float2]" }
```


### Aurora defaults

From [`Engines/Aurora.json`](../Engines/Aurora.json). Selecting Aurora on the website applies the `default` preset, then `STC`. Values below are what you should send for a normal Aurora STC SPRT.

| Field | Aurora default |
| --- | --- |
| `dev_engine` / `base_engine` | `Aurora` |
| `dev_repo` / `base_repo` | `https://github.com/kjljixx/Aurora-Chess-Engine` |
| `base_branch` | `main` |
| `dev_options` / `base_options` | `Hash=16 Threads=1` |
| `dev_time_control` / `base_time_control` | `6.0+0.06` |
| `dev_network` / `base_network` | `""` (none) |
| `dev_bench` / `base_bench` | `Autofill` |
| `book_name` | `UHO_Lichess_4852_v1.epd` |
| `upload_pgns` | `FALSE` |
| `test_mode` | `SPRT` |
| `test_bounds` | `[0.00, 10.00]` |
| `test_confidence` | `[0.1, 0.05]` |
| `workload_size` | `32` |
| `priority` | `0` |
| `throughput` | `1000` |
| `auto_schedule` | `NONE` |
| `scale_method` | `BASE` |
| `scale_nps` | `30000` |
| `syzygy_wdl` | `DISABLED` |
| `syzygy_adj` | `OPTIONAL` |
| `win_adj` | `movecount=3 score=400` |
| `draw_adj` | `movenumber=40 movecount=8 score=10` |

Other Aurora test presets (override options / TC / workload size on top of `default`):

| Preset | Options | Time control | Workload size | Extra |
| --- | --- | --- | --- | --- |
| `STC` | `Hash=16 Threads=1` | `6.0+0.06` | `32` | — |
| `LTC` | `Hash=32 Threads=1` | `20.0+0.2` | `8` | — |
| `VLTC` | `Hash=64 Threads=1` | `120.0+1.2` | `8` | — |
| `STC Simplify` | `Hash=16 Threads=1` | `6.0+0.06` | `32` | `test_bounds=[-10.00, 0.00]` |
| `LTC Simplify` | `Hash=32 Threads=1` | `20.0+0.2` | `8` | `test_bounds=[-10.00, 0.00]` |

`auto_schedule` may be set to `LTC` or `VLTC` to queue that preset after a pass.

### Required fields

#### Auth

| Field | Notes | Aurora default |
| --- | --- | --- |
| `username` | Enabled account | — |
| `password` | | — |
| `action` | Must be `CREATE_TEST` | `CREATE_TEST` |

#### Dev engine

| Field | Notes | Aurora default |
| --- | --- | --- |
| `dev_engine` | Engine key from server config | `Aurora` |
| `dev_repo` | `https://github.com/<user>/<repo>` | `https://github.com/kjljixx/Aurora-Chess-Engine` |
| `dev_branch` | Branch name, tag, or full 40-char commit SHA | *(your feature branch)* |
| `dev_bench` | Integer bench, or non-integer (e.g. `Autofill`) to parse from commit message | `Autofill` |
| `dev_network` | Network **SHA256** (8 hex), or `""` for none | `""` |
| `dev_options` | Must include `Threads=` and `Hash=` | `Hash=16 Threads=1` |
| `dev_time_control` | e.g. `6.0+0.06`, `40/10`, `N=450` | `6.0+0.06` |

#### Base engine

| Field | Notes | Aurora default |
| --- | --- | --- |
| `base_engine` | Same rules as `dev_engine` | `Aurora` |
| `base_repo` | | `https://github.com/kjljixx/Aurora-Chess-Engine` |
| `base_branch` | | `main` |
| `base_bench` | | `Autofill` |
| `base_network` | SHA256 or `""` | `""` |
| `base_options` | Must include `Threads=` and `Hash=` | `Hash=16 Threads=1` |
| `base_time_control` | | `6.0+0.06` |

#### Test settings

| Field | Notes | Aurora default |
| --- | --- | --- |
| `test_mode` | `SPRT` or `GAMES` | `SPRT` |
| `test_bounds` | SPRT only: `[elo0, elo1]` | `[0.00, 10.00]` |
| `test_confidence` | SPRT only: `[beta, alpha]` | `[0.1, 0.05]` |
| `test_max_games` | GAMES only: positive integer | *(N/A for SPRT)* |

#### Book & output

| Field | Notes | Aurora default |
| --- | --- | --- |
| `book_name` | Book filename from server config | `UHO_Lichess_4852_v1.epd` |
| `upload_pgns` | `FALSE`, `COMPACT`, or `VERBOSE` | `FALSE` |

#### Scheduling

| Field | Notes | Aurora default |
| --- | --- | --- |
| `priority` | Integer (higher runs first) | `0` |
| `throughput` | Integer **> 0** | `1000` |
| `workload_size` | Games per worker batch; integer **> 0** | `32` |
| `auto_schedule` | `NONE`, a preset name (`LTC`, `VLTC`), or `RESCHEDULE` | `NONE` |

#### Scaling

| Field | Notes | Aurora default |
| --- | --- | --- |
| `scale_method` | `DEV`, `BASE`, or `BOTH` | `BASE` |
| `scale_nps` | Expected NPS; integer **> 0** | `30000` |

#### Adjudication

| Field | Notes | Aurora default |
| --- | --- | --- |
| `syzygy_wdl` | `OPTIONAL`, `DISABLED`, or `N-MAN` | `DISABLED` |
| `syzygy_adj` | Same choices as `syzygy_wdl` | `OPTIONAL` |
| `win_adj` | `None`, or `movecount=<n> score=<n>` | `movecount=3 score=400` |
| `draw_adj` | `None`, or `movenumber=<n> movecount=<n> score=<n>` | `movenumber=40 movecount=8 score=10` |

### Optional fields

| Field | Notes |
| --- | --- |
| `workload_name` | Display name; defaults to the resolved dev branch/SHA name |

### Example (Aurora STC SPRT)

```python
import requests

URL = 'https://ob.kjljixx.com/scripts/'

data = {
    'username': 'your_user',
    'password': 'your_pass',
    'action':   'CREATE_TEST',

    'workload_name': 'my-feature-stc',

    'dev_engine':       'Aurora',
    'dev_repo':         'https://github.com/kjljixx/Aurora-Chess-Engine',
    'dev_branch':       'my-feature',
    'dev_bench':        'Autofill',
    'dev_network':      '',
    'dev_options':      'Hash=16 Threads=1',
    'dev_time_control': '6.0+0.06',

    'base_engine':       'Aurora',
    'base_repo':         'https://github.com/kjljixx/Aurora-Chess-Engine',
    'base_branch':       'main',
    'base_bench':        'Autofill',
    'base_network':      '',
    'base_options':      'Hash=16 Threads=1',
    'base_time_control': '6.0+0.06',

    'book_name':  'UHO_Lichess_4852_v1.epd',
    'upload_pgns': 'FALSE',

    'test_mode':       'SPRT',
    'test_bounds':     '[0.00, 10.00]',
    'test_confidence': '[0.1, 0.05]',
    'test_max_games':  '0',  # ignored for SPRT; still send a value

    'priority':       '0',
    'throughput':     '1000',
    'workload_size':  '32',
    'auto_schedule':  'LTC',  # or NONE / VLTC / RESCHEDULE

    'scale_method': 'BASE',
    'scale_nps':    '30000',

    'syzygy_wdl': 'DISABLED',
    'syzygy_adj': 'OPTIONAL',
    'win_adj':    'movecount=3 score=400',
    'draw_adj':   'movenumber=40 movecount=8 score=10',
}

r = requests.post(URL, data=data)
r.raise_for_status()
workload_id = r.json()['workload_id']
print('created', workload_id)
```

A starter script lives at [`Scripts/create_test.py`](../Scripts/create_test.py).

### Errors

Scripted creation returns JSON errors:

| Status | Body |
| --- | --- |
| `401` | `{ "error": "Only enabled users can create tests" }` |
| `400` | `{ "error": "<validation messages>" }` |

Website form posts to `/test/new/` still redirect to `/test/new/` on failure (HTML error block) or `/index/` on success.

Approver accounts may auto-approve the test when `use_cross_approval` is false.

---

## 2. Test State

Read aggregate progress and per-machine results for an existing workload (test, tune, or datagen).

| | |
| --- | --- |
| **URL** | `POST /api/workload/<workload_id>/results/` |
| **Content-Type** | `application/x-www-form-urlencoded` when sending credentials |
| **Response** | `application/json` |

`<workload_id>` is the integer id shown in `/test/<id>/` (same primary key for tunes/datagen).

### Request

Credentials are only required when the server has `require_login_to_view: true`:

```python
import requests

URL = 'https://ob.kjljixx.com/api/workload/1234/results/'

r = requests.post(URL, data={
    'username': 'your_user',
    'password': 'your_pass',
})
print(r.json())
```

On public instances you can omit credentials:

```python
r = requests.post(URL)
```

### Success response

```json
{
  "workload": {
    "id": 1234,
    "name": "my-feature-stc",
    "author": "your_user",
    "type": "test",
    "test_mode": "SPRT",
    "dev": "my-feature",
    "base": "main",
    "dev_engine": "Aurora",
    "base_engine": "Aurora",
    "time_control": "6.0+0.06",
    "games": 12800,
    "wins": 4100,
    "losses": 3900,
    "draws": 4800,
    "trinomial": [3900, 4800, 4100],
    "pentanomial": [100, 800, 2400, 900, 150],
    "use_penta": true,
    "elo": 5.12,
    "elo_error": 3.40,
    "currentllr": 1.25,
    "lowerllr": -2.94,
    "upperllr": 2.94,
    "elolower": 0.0,
    "eloupper": 10.0,
    "max_games": 0,
    "passed": false,
    "failed": false,
    "finished": false,
    "deleted": false,
    "approved": true,
    "awaiting": false,
    "error": false,
    "creation": 1730000000.0,
    "updated": 1730003600.0
  },
  "results": [
    {
      "machine__id": 42,
      "machine__user__username": "worker1",
      "updated": 1730000000.0,
      "games": 128,
      "wins": 40,
      "losses": 38,
      "draws": 50,
      "timeloss": 0,
      "crashes": 0,
      "active": true
    }
  ]
}
```

#### `workload` fields

| Field | Meaning |
| --- | --- |
| `id` / `name` / `author` | Workload identity |
| `type` | `test`, `tune`, or `datagen` |
| `test_mode` | `SPRT`, `GAMES`, `SPSA`, or `DATAGEN` |
| `dev` / `base` | Branch or SHA display names |
| `games` / `wins` / `losses` / `draws` | Aggregate trinomial totals |
| `trinomial` | `[losses, draws, wins]` |
| `pentanomial` | `[LL, LD, DD, DW, WW]` |
| `elo` / `elo_error` | Elo estimate and 95% half-width (`0` for SPSA) |
| `currentllr` / `lowerllr` / `upperllr` | SPRT LLR and bounds |
| `elolower` / `eloupper` | SPRT elo hypothesis bounds |
| `max_games` | Cap for `GAMES` / `DATAGEN` |
| `passed` / `failed` / `finished` | Outcome flags — poll until `finished` |
| `approved` / `awaiting` / `error` / `deleted` | Lifecycle flags |
| `creation` / `updated` | Unix timestamps |

A finished SPRT that accepted H1 has `passed=true`. A finished SPRT that accepted H0 has `failed=true`. Fixed-games tests set `finished` when `games >= max_games`.

#### `results` fields (per machine)

| Field | Meaning |
| --- | --- |
| `machine__id` | Worker machine id |
| `machine__user__username` | Account that owns the machine |
| `updated` | Unix timestamp of last result update |
| `games` / `wins` / `losses` / `draws` | Trinomial totals from that machine |
| `timeloss` / `crashes` | Fault counts |
| `active` | `true` if the machine was seen in the last minute **and** is still assigned to this workload |

All machine result rows are returned (the website UI truncates above 25).

### Error response

```json
{ "error": "API requires authentication for this server" }
```

```json
{ "error": "Requested Workload Id does not exist" }
```

### Polling example

```python
import time
import requests

URL = 'https://ob.kjljixx.com/api/workload/1234/results/'

while True:
    state = requests.post(URL).json()['workload']
    if state['finished']:
        print('passed' if state['passed'] else 'failed' if state['failed'] else 'done')
        break
    print(f"games={state['games']} llr={state['currentllr']:.2f}")
    time.sleep(60)
```

### Related read-only APIs

| Endpoint | Purpose |
| --- | --- |
| `POST /api/config/` | List engines and books |
| `POST /api/config/<engine>/` | Engine config (source, build, presets, …) |
| `POST /api/pgns/<id>/` | Download finished PGN archive (file response) |
| `POST /api/spsa/<id>/<query>/` | SPSA inputs/outputs (`inputs`, `outputs`, `outputs_ob`, `digest`, `perturbation`) |
