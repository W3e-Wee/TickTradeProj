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

Python comparison code is isolated to `src/stretch/python/` and is called only by `make stretch-python`.

For run procedures and evidence maps, see [RUNBOOK](RUNBOOK.md), [STAGE_GUIDE](STAGE_GUIDE.md), [MAKE_COMMANDS](MAKE_COMMANDS.md), [FILES_BY_STAGE](FILES_BY_STAGE.md), [PREREQUISITES](PREREQUISITES.md), and [RESULTS_LOCATIONS](RESULTS_LOCATIONS.md).

## Ladder relationships

| Version | Planned change | Status |
| --- | --- | --- |
| v0 | helper reader, double VWAP/SMA, naive SMA, `-O0` | implemented |
| v1 | v0 at `-O2` | implemented |
| v2 | v0 at `-O3` | implemented |
| v3 | optional `PI_CFLAGS` tuning | implemented; hardware tuning must be supplied and measured |
| v4 | manual decimal parser | implemented |
| v5 | compensated running-sum SMA with exact integer crossover classification | implemented |
| v6 | fixed-point VWAP/SMA and exact crossover classification | implemented |
| v7 | bounded static in-memory trades | implemented |
| v8 | count then exact-sized heap storage | implemented |
| v9_soa | stretch struct-of-arrays engine | implemented |

All ladder versions share exact scaled-integer crossover decisions to keep signal hashes identical across arithmetic and storage variants. VWAP and indicator implementations retain each rung's selected representation.
