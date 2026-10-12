# Prediction logbook template

| Stage | Prediction | Reason | Actual result | Explanation | Date | Member | Signature |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Stage 0 | The iterative sum will be stable and the recursive check will fail or be intentionally skipped for large N. | Large recursion depth is a known stack risk. | Recorded when executed. | Honest recursive guard status. | YYYY-MM-DD | Member name | Signature placeholder |
| Stage A | `test20` final VWAP should match the official checkpoint. | The dataset and expected answer supplied in the workspace indicate the correct value. | Recorded after validation. | Explain the result. | YYYY-MM-DD | Member name | Signature placeholder |
| Stage A+ | The parser should accept the valid official file and reject malformed rows. | Input validation requirements are explicit in the assignment. | Recorded after validation. | Explain the result. | YYYY-MM-DD | Member name | Signature placeholder |
| Stage 4 | The naive and reference SMA logic should agree on the official test20 data. | Crossovers are defined explicitly. | Recorded after validation. | Explain. | YYYY-MM-DD | Member name | Signature placeholder |
| Stage 5 | Build structure will be portable and cross-platform. | The project uses gcc and Python orchestration. | Recorded after build checks. | Explain. | YYYY-MM-DD | Member name | Signature placeholder |
| Stage 10 | Native AArch64 execution is not valid on this x86 host. | The laptop is not running Pi hardware. | Recorded after source generation. | Explain. | YYYY-MM-DD | Member name | Signature placeholder |
