# Third-party data notice: terminal Unicode width tables

The private terminal width tables in `src/terminal/unicode_width_tables.hpp` are generated from the
Unicode 17.0.0 tables shipped by **python-wcwidth 0.7.0**. They are not copied into the public SASD UI
Toolkit API; the pinned Unicode table version is exposed through `terminal::TextMetrics` so behavior
can be upgraded deliberately.

## python-wcwidth

python-wcwidth 0.7.0 is distributed under the MIT License:

> MIT License
>
> Copyright (c) 2017 Robpol86  
> 2025 Giacomo Battaglia
>
> Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
> associated documentation files (the "Software"), to deal in the Software without restriction,
> including without limitation the rights to use, copy, modify, merge, publish, distribute,
> sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all copies or substantial
> portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
> NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
> NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
> DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
> OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## Unicode Character Database

The upstream width/category data ultimately derives from Unicode Character Database files.
Unicode data files are distributed under the **Unicode License v3 (SPDX: Unicode-3.0)**.

Copyright © 1991-2026 Unicode, Inc.

License information: <https://www.unicode.org/license.txt>

The project intentionally pins a Unicode data version for deterministic behavior. Updating that version
must regenerate the complete private tables and rerun the terminal width/rendering contract tests.
