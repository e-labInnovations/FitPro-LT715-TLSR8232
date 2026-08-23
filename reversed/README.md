# Stock firmware as a C project

The stock LT716 firmware, decompiled and turned into C that a compiler accepts.

```bash
docker run --rm -v $(pwd)/..:/src -w /src/reversed -it tlsr8232-sdk make
# compiled 1040 functions
#    text    data     bss     dec     hex filename
#  184652       0       0  184652   2d14c _build/firmware.o
```

Zero errors, 184 KB of code from a 108 KB image. Regenerate any time with:

```bash
python3 tools/decomp2proj.py \
  ghidra_project/decompiled/LT716_V10712_211091429.c \
  binaries/stock/LT716_V10712_211091429.bin \
  reversed
```

| | |
| --- | --- |
| `src/firmware.c` | 32 117 lines, 1040 functions |
| `include/fw_data.h` | 2602 data symbols resolved to addresses or constants |
| `include/fw_regs.h` | hand-written register map — the only file not generated |
| `include/fw_decl.h` | a prototype per function |
| `include/fw_types.h` | Ghidra's types, plus the markers below |
| `function_map.txt` | address → size → name, for finding your way around |

---

## ⚠️ It builds. It will never run.

Read this before you flash anything.

The object file is real compiled TC32 code, but it cannot be the stock firmware,
for a reason that has nothing to do with how good the decompilation is:

- **Addresses moved.** The image is full of pointers to its own code —
  jump tables, callback arrays, `PTR_FUN_000086c0`. Those hold the *original*
  addresses. Recompiling puts every function somewhere else, so every one of
  those pointers now aims at the middle of something unrelated.
- **The data is not here.** Fonts, strings, bitmaps and configuration live in the
  flash image, referenced by absolute offset. This project contains code only.
- **Some semantics are gone, not just unnamed** — see the markers below.

So there is deliberately no target that produces a `.bin`. A flashable-looking
artefact here would be a trap. If you want firmware on the watch, build one of
the [examples](../examples/) — those are ours, and they work.

**What this is genuinely for:** reading. Grep it, follow a register write to the
task that does it, then reimplement that one behaviour properly. That is exactly
how [lib/vibrate](../lib/vibrate/) and [lib/touch](../lib/touch/) were written,
and it took minutes per subsystem instead of days.

---

## How the opaque parts were resolved

Ghidra's raw output is unreadable in one specific way: every hardware access is a
store through a symbol it cannot name.

```c
*PTR_DAT_0000ceb4 = *PTR_DAT_0000ceb4 | 0x20;
```

But nothing there is actually unknown. `PTR_DAT_0000ceb4` is the word at file
offset `0xceb4`, that word is `0x800583`, and `0x800583` is `PA_OUT`. Resolving
all 2602 of them and substituting the 343 that land on a known register turns the
line above into what the firmware author wrote:

```c
PA_OUT = PA_OUT | 0x20;      /* the vibrator, on */
```

Symbols come out in four shapes, decided by how the code uses each one — a
distinction that matters, because getting it wrong produces a pointer where an
integer belongs and 900 compile errors:

| Shape | Evidence | Becomes |
| ----- | -------- | ------- |
| pointer | dereferenced (`*SYM`, `SYM[2]`) | the address it holds |
| pointer variable | dereferenced *and* assigned | a pointer *at* that address |
| object | address taken (`&SYM`) | the datum itself |
| value | only ever read bare | the stored constant |

Counts for this image: 1059 pointers, 3 pointer variables, 45 objects, 1495
values, 192 of them naming a hardware register.

## Markers — where this stops being faithful

Every place the recovery is incomplete is greppable on purpose. None of these are
things the original code contained:

| Marker | Count | What it means |
| ------ | ----- | ------------- |
| `FW_UNKNOWN_ARG` | 154 | Ghidra called a function with fewer arguments than its own signature takes — the caller already had the value in the right register, so the argument is unrecovered |
| `dropped:` comment | 264 | the opposite: more arguments at the call site than in the signature. The surplus is preserved in a comment rather than deleted |
| `FW_VOID_RESULT(...)` | 205 | a value read from a function recovered as returning nothing. The callee does leave something in the return register; what it means is unknown |
| `undefined1/2/4` | 2834 | "this many bytes, and the decompiler could not tell what of" |
| `unaff_*`, `extraout_*` | 15 | register state the decompiler saw but could not attribute |

Treat any function containing one as not yet fully reversed.

`make warn` builds with warnings on: 1653 of them, mostly implicit conversions
between Ghidra's recovered types. They are a rough map of where the types are
wrong — worth reading before trusting a specific function's arithmetic.

## Named so far

12 of 1040. Each one was read in full and checked against hardware or against a
register whose purpose is already established — see `KNOWN` in
[tools/decomp2proj.py](../tools/decomp2proj.py). Add to that table and re-run;
never hand-edit the generated source.

| Name | Was | How it was identified |
| ---- | --- | --------------------- |
| `analog_write` / `analog_read` | `FUN_0000047c` / `FUN_0000043c` | the analog register interface at `0xb8`/`0xb9`, commands `0x60`/`0x40` |
| `delay_us` | `FUN_000004bc` | spins on `SYS_TICK` until `param << 4` ticks pass — 16 ticks per µs at 16 MHz |
| `tick_elapsed` | `FUN_0000ec6c` | `(param2 << 4) < SYS_TICK - param1` |
| `gpio_init_all` | `FUN_0000ed4c` | the bulk port init: `PA_OEN = 0xdf`, `PA_DS = 0x1f`, `PA_FUNC = 0xff`, pulls |
| `vibrate_task` | `FUN_0000cdc4` | toggles `PA_OUT` bit 5 on a counter, stops at 4/8/0x18 |
| `vibrate_flag_get` / `_set` | `FUN_0000aa88` / `FUN_0000aa7c` | the state byte at `+0x22` that gates the task |
| `lcd_cs_set` / `lcd_dc_set` | `FUN_00004fb4` / `FUN_00004f8c` | `PA_OUT` bit 1 and `PC_OUT` bit 1, both known display pins |
| `lcd_reset_pulse` | `FUN_000058e0` | clears `PA_OUT` bit 6, delays, sets it |
| `accel_irq_setup` | `FUN_000066e8` | PA4: FUNC/IE, output driver off, POL cleared, interrupt enabled |
