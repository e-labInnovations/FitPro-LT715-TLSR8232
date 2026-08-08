// Dumps TC32 instructions near the GPIO init literal pool and finds analog pin config.
//@category Analysis

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.*;
import java.util.*;

public class DumpInitRegWrites extends GhidraScript {

    @Override
    public void run() throws Exception {
        Listing listing = currentProgram.getListing();
        AddressFactory af = currentProgram.getAddressFactory();
        AddressSpace sp = af.getDefaultAddressSpace();
        FunctionManager fm = currentProgram.getFunctionManager();
        Memory mem = currentProgram.getMemory();

        // === PART 1: Dump literal pool values following PB_GPIO ===
        // Literal pool at 0x2000efd8:
        //   efd8: PA_IE=0x800581, efdc: PA_OEN=0x800582, efe0: PA_GPIO=0x800586
        //   efe4: PB_OEN=0x80058a, efe8: PB_GPIO=0x80058e
        //   efec: ??, eff0: ??
        println("=== Literal pool raw 32-bit values at 0x2000efec-0x2000f000 ===");
        byte[] tmp = new byte[4];
        for (long addr = 0x2000efecL; addr < 0x2000f010L; addr += 4) {
            mem.getBytes(sp.getAddress(addr), tmp);
            long val = ((long)(tmp[0]&0xff)) | ((long)(tmp[1]&0xff)<<8) |
                       ((long)(tmp[2]&0xff)<<16) | ((long)(tmp[3]&0xff)<<24);
            println(String.format("  0x%08x: 0x%08x  bytes: %02x %02x %02x %02x",
                addr, val, tmp[0]&0xff, tmp[1]&0xff, tmp[2]&0xff, tmp[3]&0xff));
        }

        // === PART 2: Dump the GPIO init function instructions (0x2000ee00-0x2000efd8) ===
        println("\n=== TC32 instructions in GPIO init region ===");
        Address start = sp.getAddress(0x2000ee00L);
        Address end   = sp.getAddress(0x2000efd8L);
        InstructionIterator iter = listing.getInstructions(start, true);
        while (iter.hasNext()) {
            Instruction instr = iter.next();
            if (instr.getAddress().compareTo(end) >= 0) break;
            StringBuilder sb = new StringBuilder();
            sb.append(String.format("  0x%08x: %-14s", instr.getAddress().getOffset(),
                instr.getMnemonicString()));
            for (int i = 0; i < instr.getNumOperands(); i++) {
                if (i > 0) sb.append(", ");
                sb.append(instr.getDefaultOperandRepresentation(i));
                Scalar s = instr.getScalar(i);
                if (s != null) {
                    long v = s.getUnsignedValue();
                    if (v != 0 && v < 0x10000000L)
                        sb.append(String.format("(=0x%x)", v));
                }
            }
            for (Reference ref : instr.getReferencesFrom()) {
                if (ref.getReferenceType().isCall()) {
                    Function cf = fm.getFunctionAt(ref.getToAddress());
                    sb.append(" → CALL " + (cf != null ? cf.getName() : ref.getToAddress()));
                } else if (ref.getReferenceType().isData()) {
                    sb.append(String.format(" → DATA[0x%08x]", ref.getToAddress().getOffset()));
                }
            }
            println(sb.toString());
        }

        // === PART 3: Find which function contains the earlier init of PA/PB GPIO regs ===
        println("\n=== Functions that reference PA_GPIO (0x800586) ===");
        // Search all instructions for references to GPIO register addresses
        long[] gpioAddrs = {0x800581L, 0x800582L, 0x800586L, 0x80058aL, 0x80058eL};
        String[] gpioNames = {"PA_IE", "PA_OEN", "PA_GPIO", "PB_OEN", "PB_GPIO"};

        // Read flash and find all 4-byte LE matches
        byte[] flash = new byte[0x40000];
        mem.getBytes(sp.getAddress(0x20000000L), flash);

        for (int gi = 0; gi < gpioAddrs.length; gi++) {
            long gaddr = gpioAddrs[gi];
            println("\n" + gpioNames[gi] + " (0x" + Long.toHexString(gaddr) + ") literal pool locations:");
            for (int off = 0; off < flash.length - 3; off += 4) {
                long w = ((long)(flash[off]&0xff)) | ((long)(flash[off+1]&0xff)<<8) |
                         ((long)(flash[off+2]&0xff)<<16) | ((long)(flash[off+3]&0xff)<<24);
                if (w == gaddr) {
                    long flashAddr = 0x20000000L + off;
                    Function f = fm.getFunctionContaining(sp.getAddress(flashAddr));
                    // The literal pool is between functions, so walk back to find the owner function
                    if (f == null) {
                        for (long back = flashAddr - 2; back >= flashAddr - 512; back -= 2) {
                            f = fm.getFunctionContaining(sp.getAddress(back));
                            if (f != null) break;
                        }
                    }

                    // Read the 4 bytes AFTER this literal (might be the write value)
                    byte[] nextLit = new byte[4];
                    if (off + 8 <= flash.length) {
                        for (int k = 0; k < 4; k++) nextLit[k] = flash[off + 4 + k];
                    }
                    long nextVal = ((long)(nextLit[0]&0xff)) | ((long)(nextLit[1]&0xff)<<8) |
                                  ((long)(nextLit[2]&0xff)<<16) | ((long)(nextLit[3]&0xff)<<24);

                    println(String.format("  0x%08x  func=%s  next_pool_word=0x%08x (%02x %02x %02x %02x)",
                        flashAddr, f != null ? f.getName() : "?",
                        nextVal, nextLit[0]&0xff, nextLit[1]&0xff, nextLit[2]&0xff, nextLit[3]&0xff));
                }
            }
        }
    }
}
