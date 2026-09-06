"""Author: Angelis Pseftis. Emit a self-contained derived review page to stdout."""
from pathlib import Path
p=Path(__file__).resolve().parent
print((p/'review-template.html').read_text().replace('__DATA__',(p/'candidate-review.json').read_text().replace('</','<\\/')).rstrip())
