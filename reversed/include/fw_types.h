/* Ghidra's decompiler types, as real C.

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
#define FW_LOAD(var, off, bytes)     (((unsigned int)(var) >> (8u * (off))) & ((bytes) >= 4 ? 0xffffffffu                                              : ((1u << (8u * (bytes))) - 1u)))
#define FW_STORE(var, off, bytes, val)                                            ((var) = (typeof(var))(((unsigned int)(var)                                             & ~((((bytes) >= 4 ? 0xffffffffu                                                   : ((1u << (8u * (bytes))) - 1u))) << (8u * (off))))                      | ((((unsigned int)(val))                                                        & ((bytes) >= 4 ? 0xffffffffu                                                   : ((1u << (8u * (bytes))) - 1u))) << (8u * (off)))))

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

#define CONCAT11(a, b) ((unsigned short)(((unsigned short)(unsigned char)(a) << 8) \
                        | (unsigned char)(b)))
#define CONCAT12(a, b) ((unsigned int)(((unsigned int)(unsigned char)(a) << 16) \
                        | (unsigned short)(b)))
#define CONCAT13(a, b) ((unsigned int)(((unsigned int)(unsigned char)(a) << 24) \
                        | ((b) & 0xffffffu)))
#define CONCAT22(a, b) ((unsigned int)(((unsigned int)(unsigned short)(a) << 16) \
                        | (unsigned short)(b)))
#define CONCAT31(a, b) ((unsigned int)(((unsigned int)(a) << 8) \
                        | (unsigned char)(b)))

#endif /* FW_TYPES_H */
