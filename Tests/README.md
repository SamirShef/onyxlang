For testing all files you need start executing of `test_all.sh`

```bash
bash test_all.sh <dir for testing>
```

Or run
```bash
chmod +x test_all.sh
/test_all.sh <dir for testing>
```

For example:
```bash
bash test_all.sh Negative
```

> [!CAUTION]
> Script requires `marblec` in PATH!

Not recommended use this script for tests in [Basic/](Basic), because some files requires CLI arguments (e.g. [Basic/Pointer.mr](Basic/Pointer.mr))
