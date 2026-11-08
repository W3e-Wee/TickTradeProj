# Stage guide

## Stretch goals

The C-versus-Python comparison is an optional Stage A stretch activity. It has
two implementations under `src/stretch/python/`: `stage_a.py` uses only the
Python standard library, and `stage_a_numpy.py` uses NumPy for vectorised
calculation. Run `make stretch-python`; the result is appended to
`results/bench/c_vs_python.csv`. Set `PYLIMIT` for a quick subset or `PYFILE`
to compare another CSV. Python and NumPy are not required by the normal C build
or test targets.
