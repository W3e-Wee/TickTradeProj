# Demo script

1. Build the debug and release binaries using `python scripts/project.py build --profile debug` and `--profile release`.
2. Run the quick test suite with `python scripts/project.py test --suite quick`.
3. Verify the official small dataset: `python scripts/project.py verify --dataset test20 --all-versions`.
4. Show the Stage 0 result and explain the recursive guard.
5. Explain that the signal rule uses `previous_delta` and `current_delta` with the crossover rules from the assignment.
6. State the remaining Pi-specific validation work honestly and explicitly.
