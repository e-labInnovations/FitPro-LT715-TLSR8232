#!/usr/bin/env python3
"""Turn Ghidra's whole-image decompilation into a buildable C project.

Ghidra's output is one flat file of 1000+ functions that references thousands of
symbols it never defines: DAT_0000ceb4, undefined1, extraout_r3. It reads as C
but no compiler will take it.

Most of that gap is mechanical, and the key realisation is that nothing is
actually unknown: every DAT_/PTR_DAT_ symbol is a literal-pool word *inside the
image*, so its value can be read straight out of the binary. DAT_0000ceb4 is the
word at file offset 0xceb4, which is 0x800583, which is PA_OUT. Do that for all
2550 of them and the sea of opaque pointers becomes named hardware registers and
concrete RAM addresses.

What this script emits (see reversed/README.md for the honest limits):

  include/fw_types.h      Ghidra's undefined1/uint/byte/code as real typedefs
  include/fw_regs.h       TLSR8232 registers by name, hand-written
  include/fw_data.h       generated: every DAT_ symbol -> address or value,
                          annotated with the register name where one matches
  include/fw_decl.h       generated: a declaration per function
  include/fw_artifacts.h  decompiler leftovers (extraout_*, in_*) as globals
  src/firmware.c          the decompilation, with known functions renamed

Usage:
  python3 tools/decomp2proj.py \
      ghidra_project/decompiled/LT716_V10712_211091429.c \
      binaries/stock/LT716_V10712_211091429.bin \
      reversed
"""

import os
import re
import struct
import sys

# ---------------------------------------------------------------- register map

REG_NAMES = {}
for _i, _p in enumerate(("PA", "PB", "PC")):
    for _j, _r in enumerate(("IN", "IE", "OEN", "OUT", "POL", "DS", "FUNC", "IRQ")):
        REG_NAMES[0x800580 + 8 * _i + _j] = "%s_%s" % (_p, _r)
REG_NAMES.update({
    0x800740: "SYS_TICK",          # free-running 16 MHz counter
    0x8000b8: "ANA_ADDR",          # analog register interface
    0x8000b9: "ANA_DATA",
    0x8000ba: "ANA_CTRL",
    0x800643: "IRQ_EN",           # SDK: reg_irq_en, the global interrupt gate
})

# Functions identified while reverse engineering. Only entries we can defend:
# each one was read in full and its behaviour matched against hardware or
# against a register whose function is already known.
KNOWN = {
    "FUN_0000043c": "analog_read",       # analog reg read via 0xb8/0xb9, cmd 0x40
    "FUN_0000047c": "analog_write",      # same interface, cmd 0x60
    "FUN_000004bc": "delay_us",          # spins on SYS_TICK until param<<4 ticks
    "FUN_0000cdc4": "vibrate_task",      # toggles PA_OUT bit5, limits 4/8/0x18
    "FUN_0000ec6c": "tick_elapsed",      # (param2<<4) < SYS_TICK - param1
    "FUN_0000ed4c": "gpio_init_all",     # bulk port init: OEN/OUT/DS/FUNC + pulls
    "FUN_0000aa88": "vibrate_flag_get",  # state byte +0x22
    "FUN_0000aa7c": "vibrate_flag_set",
    "FUN_00004fb4": "lcd_cs_set",        # PA_OUT bit1
    "FUN_00004f8c": "lcd_dc_set",        # PC_OUT bit1
    "FUN_000058e0": "lcd_reset_pulse",   # clears PA_OUT bit6, delays, sets it
    "FUN_000066e8": "accel_irq_setup",   # PA4: FUNC/IE, output off, POL, IRQ en
}


def load_image(path):
    with open(path, "rb") as fh:
        return fh.read()


def word(img, off):
    if off + 4 <= len(img):
        return struct.unpack_from("<I", img, off)[0]
    return None


def read_string(img, addr, limit=48):
    """The printable string at an image address, if there is one.

    Ghidra names a literal PTR_s_Active_Call_0001625c when it recognises what the
    pointer targets, and that name is the most readable thing in the whole dump -
    so put the actual text in the generated header too.
    """
    if addr >= len(img):
        return None
    out = []
    for b in img[addr:addr + limit]:
        if b == 0:
            break
        if 32 <= b < 127:
            out.append(chr(b))
        else:
            return None
    return "".join(out).replace("*/", "* /") if len(out) > 1 else None


def classify(src):
    """How is each generated symbol used: pointer, object, or plain value?

    Ghidra's data symbols all end in the hex address they came from, however
    decorated: DAT_0000ceb4, PTR_PTR_00007530, PTR_LAB_00009066_3_00000844. The
    trailing group is the one that matters - it is where the word lives.

    Three usages have to be told apart:
      pointer  dereferenced (`*SYM`, `*(int *)SYM`, `SYM[2]`) - holds an address
      object   assigned or address-taken (`SYM = 1`, `&SYM`)  - is the datum
      value    only ever read bare                            - is a constant

    The deref test has to reject multiplication: in `iVar1 * DAT_00001234` the
    star is a binary operator, not a dereference, and reading it as one produces
    a pointer where an integer belongs.
    """
    # Ghidra decorates these names with whatever it inferred: PTR_DAT_,
    # PTR_s_Active_Call_ (a pointer to a string it recognised), puRAM000101xx
    # (a typed location). The address is always the trailing hex group.
    sym_re = re.compile(r'\b((?:PTR_)*(?:DAT|PTR|LAB|UNK|FUN)_[0-9a-fA-F]{4,8}'
                        r'(?:_[0-9]+_[0-9a-fA-F]{4,8})?'
                        r'|PTR_[A-Za-z0-9_]*?[0-9a-fA-F]{8}'
                        r'|[a-z]{0,2}RAM[0-9a-fA-F]{8})\b')
    # Collect evidence per symbol first, decide after: a single occurrence
    # cannot tell you what a symbol is. `*SYM = v` is a store *through* SYM, so
    # it is evidence of a pointer, not of SYM being assigned.
    flags = {}
    for m in sym_re.finditer(src):
        name = m.group(1)
        # LAB_/UNK_/FUN_ on their own are code, not data: defining LAB_00000980
        # turns its label into a constant and every goto into a syntax error.
        if name.startswith(("LAB_", "UNK_", "FUN_")):
            continue
        before = src[max(0, m.start() - 16):m.start()]
        after = src[m.end():m.end() + 3]
        f = flags.setdefault(name, {"deref": False, "assign": False, "addr": False})

        # strip intervening casts: "*(int *)SYM", "(uint)(byte)*SYM"
        stripped = before
        while True:
            shorter = re.sub(r'\(\s*\w+\s*\**\s*\)\s*$', '', stripped)
            if shorter == stripped:
                break
            stripped = shorter

        deref = False
        if stripped.rstrip().endswith('*'):
            head = stripped.rstrip()[:-1].rstrip()
            while True:
                shorter = re.sub(r'\(\s*\w+\s*\**\s*\)\s*$', '', head)
                if shorter == head:
                    break
                head = shorter
            # a star is a dereference unless a *value* precedes it - but
            # `return *DAT_x` and `case *DAT_x` end in a keyword, not a value
            kw = re.search(r'\b(return|case|else|do|while|if|and|or)\s*$', head)
            deref = kw is not None or not (
                head and (head[-1].isalnum() or head[-1] in ')]_'))
        if after.startswith('['):
            deref = True

        if deref:
            f["deref"] = True
        elif before.rstrip().endswith('&'):
            f["addr"] = True
        elif re.match(r'\s*(=[^=]|\+\+|--)', after):
            f["assign"] = True

    kinds = {}
    for name, f in flags.items():
        if f["deref"] and f["assign"]:
            # dereferenced here, assigned there: the literal is the address of a
            # pointer variable, not the address it points to
            kinds[name] = "pointer_object"
        elif f["deref"]:
            kinds[name] = "pointer"
        elif f["addr"] or f["assign"]:
            kinds[name] = "object"
        else:
            kinds[name] = "value"
    return kinds


def sym_addr(name):
    """The address a generated symbol name encodes: its last hex group."""
    m = re.search(r'([0-9a-fA-F]{4,8})$', name)
    return int(m.group(1), 16)


def gen_data_header(img, kinds):
    lines = [
        "/* Generated by tools/decomp2proj.py - do not edit.",
        " *",
        " * Every symbol below is a literal-pool word inside the flash image: the",
        " * name's hex suffix is the file offset, and the value stored there is what",
        " * the code loads. So DAT_0000ceb4 is the word at 0xceb4, which is 0x800583,",
        " * which is PA_OUT - the vibrator's port register.",
        " *",
        " * Three shapes, decided by how the decompilation uses each symbol:",
        " *   pointer  dereferenced somewhere  -> cast of the stored address",
        " *   object   taken by address (&SYM) -> the datum at the symbol itself",
        " *   value    only ever read bare     -> the stored constant",
        " */",
        "",
        "#ifndef FW_DATA_H",
        "#define FW_DATA_H",
        "",
    ]
    stats = {"pointer": 0, "pointer_object": 0, "object": 0, "value": 0,
             "unresolved": 0}
    named = 0
    for name in sorted(kinds):
        kind = kinds[name]
        addr = sym_addr(name)
        val = word(img, addr)
        if val is None:
            # Not a literal in the image: the symbol names a location in the
            # register space or SRAM directly, so it is the datum itself.
            reg = REG_NAMES.get(addr)
            lines.append("#define %-24s (*(volatile unsigned char *)0x%08xu)%s"
                         % (name, addr, "  /* %s */" % reg if reg else ""))
            stats["unresolved"] += 1
            continue
        # typed RAM symbols name a location directly and carry their own type
        m_ram = re.fullmatch(r'([a-z]{0,2})RAM([0-9a-fA-F]{8})', name)
        if m_ram:
            ctype = {"i": "int", "u": "unsigned int", "c": "char",
                     "s": "short", "b": "unsigned char", "": "unsigned char",
                     "pu": "unsigned char *", "pi": "int *", "pc": "char *",
                     "ps": "short *", "pb": "unsigned char *"}
            lines.append("#define %-24s (*(volatile %s *)0x%08xu)"
                         % (name, ctype.get(m_ram.group(1), "unsigned char"), addr))
            stats["object"] += 1
            continue
        comment = ""
        if kind in ("pointer", "pointer_object") and val in REG_NAMES:
            comment = "  /* %s */" % REG_NAMES[val]
            named += 1
        elif name.startswith("PTR_s_"):
            text = read_string(img, val)
            if text:
                comment = '  /* "%s" */' % text
        if kind == "pointer":
            lines.append("#define %-24s ((volatile unsigned char *)0x%08xu)%s"
                         % (name, val, comment))
        elif kind == "pointer_object":
            lines.append("#define %-24s (*(volatile unsigned char **)0x%08xu)"
                         % (name, val))
        elif kind == "object":
            lines.append("#define %-24s (*(volatile unsigned char *)0x%08xu)"
                         % (name, addr))
        else:
            lines.append("#define %-24s (0x%08xu)" % (name, val))
        stats[kind] += 1
    lines += ["", "#endif /* FW_DATA_H */", ""]
    return "\n".join(lines), stats, named


def signatures(src):
    """Parse each function definition: return type, parameters, and where it is.

    Anchored on the `/* ======== NAME @ addr ======== */` markers the dump is
    built from, not on a signature pattern - Ghidra wraps long parameter lists
    across lines, and a regex that misses one silently reports the function as
    taking no arguments, which then makes every real call to it look wrong.

    Returns (sigs, def_sites) where def_sites holds the source offset of each
    definition's name, so the call rewriter can tell definitions from calls.
    """
    sigs, def_sites = {}, set()
    marker = re.compile(r'/\* ======== (FUN_[0-9a-f]{4,8}) @ [0-9a-f]+'
                        r'  size=\d+ ======== \*/\n(.*?)\n\{', re.S)
    for m in marker.finditer(src):
        name, sig = m.group(1), m.group(2)
        i = sig.find(name)
        if i < 0:
            continue
        ret = sig[:i].strip()
        try:
            popen = sig.index('(', i + len(name))
        except ValueError:
            continue
        args, _ = split_args(sig, popen)
        args = [a.strip() for a in args if a.strip() not in ("", "void")]
        sigs[name] = (ret, args)
        def_sites.add(m.start(2) + i)
    return sigs, def_sites


def gen_decl_header(sigs):
    """A prototype per function, matching its definition exactly.

    Empty-parenthesis declarations would be more forgiving of Ghidra's
    inconsistent call sites, but they collide with any definition taking a char
    or short parameter (default promotion makes the two types differ), which is
    269 of these functions. So: exact prototypes, and the call sites get fixed
    up instead - see fix_calls.
    """
    lines = ["/* Generated by tools/decomp2proj.py - do not edit. */",
             "", "#ifndef FW_DECL_H", "#define FW_DECL_H", ""]
    for name in sorted(sigs):
        ret, args = sigs[name]
        lines.append("%s %s(%s);" % (ret, KNOWN.get(name, name),
                                     ", ".join(args) if args else "void"))
    lines += ["", "#endif /* FW_DECL_H */", ""]
    return "\n".join(lines), len(sigs)


def split_args(text, start):
    """Balanced-paren split of a call's arguments. `start` indexes the '('."""
    depth, i, args, cur = 0, start, [], ""
    while i < len(text):
        c = text[i]
        if c in "([":
            depth += 1
            if depth == 1:
                i += 1
                continue
        elif c in ")]":
            depth -= 1
            if depth == 0:
                if cur.strip():
                    args.append(cur.strip())
                return args, i
        if depth == 1 and c == ',':
            args.append(cur.strip())
            cur = ""
        else:
            cur += c
        i += 1
    return args, i


VALUE_CONTEXT = re.compile(r'(?:[=+\-*/%<>!&|^,(?:\[]|\breturn)\s*$')


def fix_calls(src, sigs, def_sites):
    """Reconcile Ghidra's call sites with its own recovered signatures.

    Two disagreements, both artefacts of register-passing rather than of the
    original code:

      arity   the same function is called with 0 and with 2 arguments, because
              the caller already had the value in the right register. Missing
              arguments become FW_UNKNOWN_ARG; surplus ones are dropped into a
              comment so nothing is silently lost.

      result  a value is taken from a call to a function recovered as void. The
              callee does leave something in the return register, so the value is
              real but unrecovered: FW_VOID_RESULT() makes that explicit.

    Both markers are greppable on purpose - they mark exactly the places where
    this project stops being a faithful account of the firmware.
    """
    counts = {"padded": 0, "trimmed": 0, "void_result": 0}
    names = sorted(sigs, key=len, reverse=True)
    name_re = re.compile(r'\b(' + "|".join(names) + r')\s*\(')
    out, pos = [], 0
    while True:
        m = name_re.search(src, pos)
        if not m:
            out.append(src[pos:])
            break
        name = m.group(1)
        ret, want = sigs[name]
        args, end = split_args(src, m.end() - 1)

        # a definition, not a call
        if m.start() in def_sites:
            out.append(src[pos:end + 1])
            pos = end + 1
            continue
        line_start = src.rfind("\n", 0, m.start()) + 1

        dropped = ""
        if len(args) < len(want):
            args += ["FW_UNKNOWN_ARG"] * (len(want) - len(args))
            counts["padded"] += 1
        elif len(args) > len(want):
            dropped = " /* dropped: %s */" % ", ".join(args[len(want):])
            args = args[:len(want)]
            counts["trimmed"] += 1

        call = "%s(%s)%s" % (name, ", ".join(args), dropped)

        before = src[max(0, m.start() - 200):m.start()]
        if ret == "void" and VALUE_CONTEXT.search(before.replace("\n", " ")):
            call = "FW_VOID_RESULT(%s)" % call
            counts["void_result"] += 1

        out.append(src[pos:m.start()])
        out.append(call)
        pos = end + 1
    return "".join(out), counts


def fix_subfields(src):
    """Rewrite Ghidra's byte-slice syntax into real loads and stores.

    `uStack_14._1_3_` means the three bytes at offset 1 of that variable - the
    decompiler's way of showing a register being filled a piece at a time. It is
    not C, so it becomes FW_LOAD/FW_STORE with the offset and width spelled out.
    """
    n_store = n_load = 0

    def store(m):
        nonlocal n_store
        n_store += 1
        return "FW_STORE(%s, %s, %s, %s);" % (m.group(1), m.group(2), m.group(3),
                                              m.group(4).strip())

    # `=(?!=)` matters: `param_5._2_2_ == 8` is a comparison, and rewriting it
    # as a store swallows the rest of the expression and unbalances the function.
    src, n_store = re.subn(r'\b(\w+)\._(\d)_(\d)_\s*=(?!=)\s*([^;]*);', store, src)
    src, n_load = re.subn(r'\b(\w+)\._(\d)_(\d)_',
                          lambda m: "FW_LOAD(%s, %s, %s)" % (m.group(1), m.group(2),
                                                             m.group(3)), src)
    return src, n_store, n_load


def gen_artifacts_header(src):
    """Decompiler leftovers, declared so the file compiles.

    extraout_r3 means "whatever r3 held after that call" - a value the decompiler
    could see but not name. in_r3 is the mirror case on entry. These are honest
    gaps in the recovery, not variables the original code had, so they get
    declared and flagged rather than guessed at.
    """
    names = sorted(set(re.findall(r'\b(?:extraout|in|joined|unaff)_\w+', src))
                   | set(re.findall(r'\bstack0x[0-9a-f]+', src))
                   | set(re.findall(r'\b_local_[0-9a-f]+', src)))
    # `code *in_r3;` is declared locally by Ghidra, so skip anything so declared
    local = set(re.findall(r'\b\w+ \*?(\w*(?:extraout|in|joined)_\w+);', src))
    names = [n for n in names if n not in local]
    lines = ["/* Generated by tools/decomp2proj.py - do not edit.",
             " *",
             " * Unrecovered register state. Each of these is a place where the",
             " * decompiler knew a value existed but not where it came from, so",
             " * nothing here carries meaning at runtime - treat any function that",
             " * reads one as not yet fully reversed.",
             " */",
             "", "#ifndef FW_ARTIFACTS_H", "#define FW_ARTIFACTS_H", ""]
    for n in names:
        lines.append("extern int %s;" % n)
    lines += ["", "#endif /* FW_ARTIFACTS_H */", ""]
    return "\n".join(lines), names


def fix_trailing_labels(src):
    """A label with nothing after it is not a statement in C.

    Ghidra emits `LAB_00001234:` immediately before a closing brace when the
    only thing at that address was the function's own epilogue. An empty
    statement makes it legal without changing anything.
    """
    return re.subn(r'((?:LAB|joined)_[0-9a-fx]+:)(\s*\})', r'\1 ;\2', src)


def inline_registers(src, img, kinds):
    """Replace resolved register dereferences with the register's name.

    This is the difference between

        *PTR_DAT_0000ceb4 = *PTR_DAT_0000ceb4 | 0x20;
    and
        PA_OUT = PA_OUT | 0x20;              /* the vibrator, on */

    Only the plain dereference forms are substituted. Where the code keeps the
    pointer in a local and works through that, the name cannot follow it, so the
    fw_data.h define stays for those.
    """
    subs = 0
    for name, kind in sorted(kinds.items(), key=lambda kv: -len(kv[0])):
        if kind not in ("pointer", "pointer_object"):
            continue
        val = word(img, sym_addr(name))
        reg = REG_NAMES.get(val) if val is not None else None
        if not reg:
            continue
        # `*(uint *)SYM` - a 32-bit read, which is how SYS_TICK is used
        src, n1 = re.subn(r'\*\s*\(\s*\w+\s*\*\s*\)\s*' + name + r'\b', reg, src)
        src, n2 = re.subn(r'\*\s*' + name + r'\b', reg, src)
        subs += n1 + n2
    return src, subs


def rename(src):
    for old, new in KNOWN.items():
        src = re.sub(r'\b%s\b' % old, new, src)
    return src


TYPES_H = """/* Ghidra's decompiler types, as real C.

Names like `undefined4` mean "4 bytes, and the decompiler could not tell what
of" - they are a statement about missing information, not about the hardware.
Anywhere one survives in the output, the type there is still unknown.
*/

#ifndef FW_TYPES_H
#define FW_TYPES_H

typedef unsigned char  undefined;
typedef unsigned char  undefined1;
typedef unsigned short undefined2;
typedef unsigned int   undefined4;
typedef unsigned char  byte;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;
typedef unsigned int   undefined3;   /* a 3-byte slice, widened */
/* Ghidra's 3-byte integer. Widened to 4, so `*(uint3 *)p` reads one byte more
   than the firmware did - harmless for reading the code, wrong if you rely on
   it. Every use is a packed 24-bit field. */
typedef unsigned int   uint3;
typedef int            int3;
typedef int            bool;

#define true  1
#define false 0

/* Ghidra shows a register being filled piecemeal as `var._1_3_` - the three
   bytes at offset 1. These make that a real load and store. */
#define FW_LOAD(var, off, bytes) \
    (((unsigned int)(var) >> (8u * (off))) & ((bytes) >= 4 ? 0xffffffffu \
                                             : ((1u << (8u * (bytes))) - 1u)))
#define FW_STORE(var, off, bytes, val)                                        \
    ((var) = (typeof(var))(((unsigned int)(var)                               \
              & ~((((bytes) >= 4 ? 0xffffffffu                                \
                   : ((1u << (8u * (bytes))) - 1u))) << (8u * (off))))        \
              | ((((unsigned int)(val))                                       \
                 & ((bytes) >= 4 ? 0xffffffffu                                \
                   : ((1u << (8u * (bytes))) - 1u))) << (8u * (off)))))

/* An argument the decompiler could not recover: the callee reads a register the
   caller never visibly set. Every one of these is a gap in the reversing, not a
   constant the original code passed. */
#define FW_UNKNOWN_ARG 0

/* A value taken from a call to a function recovered as returning nothing. The
   callee does leave something in the return register, so the value is real -
   just not recovered. */
#define FW_VOID_RESULT(call) ((call), 0)

/* `code` is Ghidra's "there is executable code here" type. As a typedef for a
   function it makes `code *` a function pointer, which is what the indirect
   calls through the vtable-like tables in this image actually are. It returns
   int because the call sites read a result; whether a given target really
   returns one is unknown. */
typedef int code(void);

#define CONCAT11(a, b) ((unsigned short)(((unsigned short)(unsigned char)(a) << 8) \\
                        | (unsigned char)(b)))
#define CONCAT12(a, b) ((unsigned int)(((unsigned int)(unsigned char)(a) << 16) \\
                        | (unsigned short)(b)))
#define CONCAT13(a, b) ((unsigned int)(((unsigned int)(unsigned char)(a) << 24) \\
                        | ((b) & 0xffffffu)))
#define CONCAT22(a, b) ((unsigned int)(((unsigned int)(unsigned short)(a) << 16) \\
                        | (unsigned short)(b)))
#define CONCAT31(a, b) ((unsigned int)(((unsigned int)(a) << 8) \\
                        | (unsigned char)(b)))

#endif /* FW_TYPES_H */
"""

REGS_H = """/* TLSR8232 hardware registers, by name.

Hand-written, not generated: this is the map that makes fw_data.h readable. The
address space is not part of the flash image, which is why the decompiler could
only ever show these as stores through pointers.

OEN is active LOW: a zero bit enables that pin's output driver. The stock
firmware writes PA_OEN = 0xdf, which enables exactly one pin - PA5, the
vibrator motor.
*/

#ifndef FW_REGS_H
#define FW_REGS_H

#define REG8(off)  (*(volatile unsigned char *)(0x800000u + (off)))
#define REG32(off) (*(volatile unsigned int  *)(0x800000u + (off)))

#define PA_IN    REG8(0x580)
#define PA_IE    REG8(0x581)
#define PA_OEN   REG8(0x582)
#define PA_OUT   REG8(0x583)
#define PA_POL   REG8(0x584)
#define PA_DS    REG8(0x585)
#define PA_FUNC  REG8(0x586)
#define PA_IRQ   REG8(0x587)

#define PB_IN    REG8(0x588)
#define PB_IE    REG8(0x589)
#define PB_OEN   REG8(0x58a)
#define PB_OUT   REG8(0x58b)
#define PB_POL   REG8(0x58c)
#define PB_DS    REG8(0x58d)
#define PB_FUNC  REG8(0x58e)
#define PB_IRQ   REG8(0x58f)

#define PC_IN    REG8(0x590)
#define PC_IE    REG8(0x591)
#define PC_OEN   REG8(0x592)
#define PC_OUT   REG8(0x593)
#define PC_POL   REG8(0x594)
#define PC_DS    REG8(0x595)
#define PC_FUNC  REG8(0x596)
#define PC_IRQ   REG8(0x597)

#define SYS_TICK REG32(0x740)   /* free-running, 16 MHz - SDK reg_system_tick */
#define IRQ_EN   REG8(0x643)    /* global interrupt gate - SDK reg_irq_en */

#define ANA_ADDR REG8(0x0b8)    /* analog register interface */
#define ANA_DATA REG8(0x0b9)
#define ANA_CTRL REG8(0x0ba)

/* Pins established by reverse engineering - see the README pin inventory. */
#define PIN_VIBRATE_BIT   0x20  /* PA5, active high, battery power only */
#define PIN_TOUCH_BIT     0x04  /* PC2, active high */
#define PIN_BACKLIGHT_BIT 0x08  /* PB3 */
#define PIN_LCD_CS_BIT    0x02  /* PA1 */
#define PIN_LCD_RST_BIT   0x40  /* PA6 */
#define PIN_LCD_DC_BIT    0x02  /* PC1 */
#define PIN_ACCEL_IRQ_BIT 0x10  /* PA4 */

#endif /* FW_REGS_H */
"""


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    dump_path, img_path, outdir = sys.argv[1:4]

    src = open(dump_path).read()
    img = load_image(img_path)

    # drop Ghidra's leading index block; it is regenerated as a map file
    body_start = src.find("/* ======== ")
    index = src[:body_start]
    body = src[body_start:]

    body, n_store, n_load = fix_subfields(body)
    body, n_labels = fix_trailing_labels(body)
    sigs, def_sites = signatures(body)
    body, callfix = fix_calls(body, sigs, def_sites)

    kinds = classify(body)
    body, n_regs = inline_registers(body, img, kinds)
    data_h, stats, named = gen_data_header(img, kinds)
    decl_h, nfuncs = gen_decl_header(sigs)
    art_h, artifacts = gen_artifacts_header(body)

    inc = os.path.join(outdir, "include")
    srcd = os.path.join(outdir, "src")
    os.makedirs(inc, exist_ok=True)
    os.makedirs(srcd, exist_ok=True)

    open(os.path.join(inc, "fw_types.h"), "w").write(TYPES_H)
    open(os.path.join(inc, "fw_regs.h"), "w").write(REGS_H)
    open(os.path.join(inc, "fw_data.h"), "w").write(data_h)
    open(os.path.join(inc, "fw_decl.h"), "w").write(decl_h)
    open(os.path.join(inc, "fw_artifacts.h"), "w").write(art_h)

    prologue = '''/* Stock FitPro LT716 firmware, decompiled and made to compile.
 *
 * Generated by tools/decomp2proj.py from the Ghidra decompilation of
 * binaries/stock/LT716_V10712_211091429.bin (md5 a52d9e46). Do not edit by
 * hand - re-run the generator, or add a name to its KNOWN table.
 *
 * Read reversed/README.md before drawing conclusions from anything here.
 */

#include "fw_types.h"
#include "fw_regs.h"
#include "fw_data.h"
#include "fw_decl.h"
#include "fw_artifacts.h"

'''
    open(os.path.join(srcd, "firmware.c"), "w").write(prologue + rename(body))
    open(os.path.join(outdir, "function_map.txt"), "w").write(rename(index))

    print("functions:      %d (%d named)" % (nfuncs, len(KNOWN)))
    print("data symbols:   %d pointer, %d pointer-var, %d object, %d value, "
          "%d direct" % (stats["pointer"], stats["pointer_object"],
                         stats["object"], stats["value"], stats["unresolved"]))
    print("                %d resolved to a named hardware register" % named)
    print("registers:      %d dereferences replaced with the register name" % n_regs)
    print("artifacts:      %d unrecovered register values" % len(artifacts))
    print("byte slices:    %d stores, %d loads rewritten" % (n_store, n_load))
    print("labels:         %d trailing labels given a statement" % n_labels)
    print("call sites:     %d padded, %d trimmed, %d void results"
          % (callfix["padded"], callfix["trimmed"], callfix["void_result"]))


if __name__ == "__main__":
    main()
