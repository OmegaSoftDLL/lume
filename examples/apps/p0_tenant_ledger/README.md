# P0 tenant ledger: real HTTP and persistence slice

This is a bounded Linux x86-64 acceptance increment for #337/#332, not a production SaaS or Web
platform. The server is Lume compiled to a native ELF. It accepts sequential TCP connections and
serves JSON over plain HTTP/1.1 on `127.0.0.1:18737`.

## Real behavior

- `GET /health` returns process health.
- `GET /v1/balance` returns only the authenticated tenant's computed balance.
- `POST /v1/debit` and `POST /v1/credit` take an integer body and append a WAL snapshot through the
  existing `persist_tx_insert_row` primitive (`flock`, `write`, `fsync`, `close`).
- The demonstration bearer token must match `X-Tenant-ID`: tenant `101` uses
  `Bearer ledger-101-token`; tenant `202` uses `Bearer ledger-202-token`.
- Missing/invalid credentials, cross-tenant reads and writes, duplicate `Content-Length`,
  `Transfer-Encoding`, invalid lengths, bodies over 1024 bytes, invalid amounts, and overdrafts are
  rejected. Each accepted request is closed after one response.
- Restarting the process recovers balances from `p0_tenant_ledger.wal` in its working directory.
  Invalid trailing frames are ignored in favor of the newest valid committed snapshot.

The tokens are deliberately demo credentials, not a session system or secret-management solution.
The ledger starts both tenants at 100 units and stores signed deltas; it is not a general-purpose
accounting database.

## Build and verify

From the repository root:

```sh
./lume build examples/apps/p0_tenant_ledger/ledger_server.src -o /tmp/ledger_server
python3 tests/capstones/test_saas_http.py /tmp/ledger_server
```

The test is an independent Python `http.client`/raw-socket client. It checks real status codes and
JSON bodies, adversarial cross-tenant requests, malformed framing, persistence across process
restarts, and torn-frame recovery. It does not use Lume's HTTP client or parser to validate itself.

## Explicitly open

TLS and independent TLS interoperability; complete TCP behavior; timeouts and backpressure;
multi-worker async I/O, cancellation, graceful shutdown and load; production authentication,
sessions, secret handling and rate limits; conditional concurrent transactions; migrations,
backup/restore, observability and deployment; and every browser/DOM criterion in #333. No browser
artifact is included. These gaps keep full #332/#333 acceptance open.
