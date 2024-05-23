# FredC vs. JSON

A single-file C library for parsing, processing, and stringifying JSON data.

## Features

- `fredc.h`
    - Parse JSON strings into a tree of fredc objects.
    - Modify (get, set, free, etc) existing fredc objects
    - Convert fredc objects and properties back to nicely formatted JSON strings.

FredC vs. JSON doesn't care if you have trailing commas in your objects,
but its stringify functions will correctly ommit trailing commas.

## Quick Start

Define `FREDC_IMPLEMENTATION` and include `fredc.h` in one (**and only one**) translation unit in your project.

```c
// fredc.c
#define FREDC_IMPLEMENTATION
#include "fredc.h"
```

Then include `fredc.h` as a normal header file anywhere else it is used.

## Contributing

If you'd like to contribute, please fork the repository and open a pull request.

