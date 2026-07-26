---
name: rpmsgclientsample
description: rpmsgclientsample demonstrates the use of OpenAMP rpmsg on the linux side.
---

## Build

As user torizon:

```bash
cd rpmsgclientsample
/usr/bin/aarch64-tdx-linux-gcc -Wall -Wextra rpmsgclientsample.c -o rpmsgclientsample
```

## Run rpmsgclientsample

As user root: `rpmsgclientsample/rpmsgclientsample`
