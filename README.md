# TuxArena

TuxArena is a fast-paced, 90s-style top-down shooter featuring iconic open-source mascots.

## How to Run

### Client

To run the game as a client, simply execute the `tuxarena` binary:

```bash
./build/bin/tuxarena
```

By default, the client will attempt to connect to a server running on `127.0.0.1:12345`.

### Server

To run a dedicated server, use the `--server` flag:

```bash
./build/bin/tuxarena --server
```

You can also specify a port and map for the server:

```bash
./build/bin/tuxarena --server --port 12345 --map arena1.tmx
```
