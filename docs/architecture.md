# Architecture

```text
              stage programs / tools / tests
                            |
           +----------------+----------------+
           |                |                |
      indicators       signal_rule       fixed_point
           |                |                |
           +------ shared common APIs ------+
                            |
              +------------+------------+
              |                         |
       platform.c timers           replay modules
       clock / sleep / ANSI       scheduler / engine
       affinity / memory          event queue / display
              |                         |
              +----------+--------------+
                         |
                  provided helpers
             now_ns / sleep_ms / LED
```

Dependency rule: `indicators`, `signal_rule`, and `fixed_point` depend only on
portable compiler compatibility definitions. Programs depend on modules;
modules never depend on programs. OS-specific headers and calls belong only in
`src/common/platform.c`.

Stage C uses `platform.c` for timer resolution, measured sleep granularity,
affinity and terminal setup. `replay.c` schedules trade release, `replay_engine.c`
updates each strategy, and `replay_events.c` queues deferred signal rendering
through `signal_display.c`.

## Ladder relationships

| Version | Planned change | Status |
| --- | --- | --- |
| v0 | helper reader, double VWAP/SMA, naive SMA, `-O0` | implemented |
| v1 | v0 at `-O2` | implemented |
| v2 | v0 at `-O3` | implemented |
| v3 | optional `PI_CFLAGS` tuning | implemented; hardware tuning must be supplied and measured |
| v4 | manual decimal parser | implemented |
| v5 | compensated running-sum SMA | implemented |
| v6 | fixed-point VWAP/SMA | implemented |
| v7 | bounded static in-memory trades | implemented |
| v8 | count then exact-sized heap storage | implemented |
| v9_soa | stretch struct-of-arrays engine | implemented |

All ladder versions share exact scaled-integer crossover decisions to keep signal hashes identical across arithmetic and storage variants. VWAP and indicator implementations retain each rung's selected representation.
