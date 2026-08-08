// Deep battery ADC analysis: dump the battery function, find the ADC read call.
//@category Analysis

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.*;
import java.util.*;

public class BatteryDeep extends GhidraScript {

    @Override
    public void run() throws Exception {
        Listing listing = currentProgram.getListing();
        AddressFactory af = currentProgram.getAddressFactory();
        AddressSpace sp = af.getDefaultAddressSpace();
        FunctionManager fm = currentProgram.getFunctionManager();
        Memory mem = currentProgram.getMemory();

        // --- 1. Find function that contains the 3500mV literal ---
        // LE16 0x0dac is at flash 0x20003f62-0x20003f63.
        // Walk backwards from 0x3f62 to find the enclosing function entry.
        println("=== Hunting battery function around 0x20003f62 ===");
        for (long a = 0x20003f62L; a >= 0x20003000L; a -= 2) {
            Function f = fm.getFunctionContaining(sp.getAddress(a));
            if (f != null) {
                println("Function found: " + f.getName() + " @ " + f.getEntryPoint());
                dumpFunction(f, listing, sp, fm);
                break;
            }
        }

        // --- 2. Dump FUN_2000802c (battery percent, 70/90 thresholds) ---
        println("\n=== FUN_2000802c (battery percent function) ===");
        Function fn802c = fm.getFunctionAt(sp.getAddress(0x2000802cL));
        if (fn802c != null) dumpFunction(fn802c, listing, sp, fm);
        else println("(not found as function entry — may be named differently)");

        // --- 3. Dump tiny functions in the 0x2000a340-0x2000a380 cluster ---
        println("\n=== ADC candidate cluster (0x2000a340-0x2000a380) ===");
        long[] candidates = {0x2000a344L, 0x2000a354L, 0x2000a364L, 0x2000a374L, 0x2000a2a0L};
        for (long addr : candidates) {
            Function f = fm.getFunctionAt(sp.getAddress(addr));
            if (f == null) f = fm.getFunctionContaining(sp.getAddress(addr));
            if (f != null) {
                println("\n-- " + f.getName() + " @ " + f.getEntryPoint() + " --");
                dumpFunction(f, listing, sp, fm);
            }
        }

        // --- 4. Dump FUN_20001768 (overlaps PWM register ref) ---
        println("\n=== FUN_20001768 (PWM/ADC overlap area) ===");
        Function fn1768 = fm.getFunctionAt(sp.getAddress(0x20001768L));
        if (fn1768 == null) fn1768 = fm.getFunctionContaining(sp.getAddress(0x20001768L));
        if (fn1768 != null) dumpFunction(fn1768, listing, sp, fm);

        // --- 5. Check PA_IE/PA_GPIO values written at boot ---
        // Scan all stores to PA_IE (0x800581) and PA_GPIO (0x800586) and record values
        // by looking at the instruction before each store for a mov #imm
        println("\n=== PA_IE and PA_GPIO values set in firmware ===");
        long PA_IE_ADDR  = 0x800581L;
        long PA_GPIO_ADDR = 0x800586L;
        byte[] flash = new byte[0x40000];
        mem.getBytes(sp.getAddress(0x20000000L), flash);

        for (int off = 0; off < flash.length - 3; off += 4) {
            long word = ((long)(flash[off] & 0xff)) |
                        ((long)(flash[off+1] & 0xff) << 8) |
                        ((long)(flash[off+2] & 0xff) << 16) |
                        ((long)(flash[off+3] & 0xff) << 24);
            if (word == PA_IE_ADDR || word == PA_GPIO_ADDR) {
                String regName = (word == PA_IE_ADDR) ? "PA_IE" : "PA_GPIO";
                long flashAddr = 0x20000000L + off;
                // Look at instructions in range [flashAddr-32, flashAddr] for mov rN,#imm
                for (int lb = 2; lb <= 32; lb += 2) {
                    int prevOff = off - lb;
                    if (prevOff < 0) break;
                    int b0 = flash[prevOff] & 0xff;
                    int b1 = flash[prevOff+1] & 0xff;
                    if (b1 >= 0x20 && b1 <= 0x27) { // mov rN, #imm8
                        int rn = b1 - 0x20;
                        int imm = b0;
                        println(String.format("  %s literal@0x%08x: nearby mov r%d,#0x%02x (%s)",
                            regName, flashAddr, rn, imm, toBinary(imm, 8)));
                    }
                }
            }
        }
        println("\n(PA_IE bit=0 or PA_GPIO bit=0 → pin is in analog/alt-function mode)");
        println("Pins with IE=0 AND GPIO=0 are the analog ADC input candidates.");
    }

    void dumpFunction(Function f, Listing listing, AddressSpace sp, FunctionManager fm) {
        println("  Size: " + f.getBody().getNumAddresses() + " bytes");
        InstructionIterator iter = listing.getInstructions(f.getBody(), true);
        int count = 0;
        while (iter.hasNext() && count < 60) {
            Instruction instr = iter.next();
            StringBuilder line = new StringBuilder();
            line.append(String.format("  0x%08x: %-12s", instr.getAddress().getOffset(),
                instr.getMnemonicString()));
            for (int i = 0; i < instr.getNumOperands(); i++) {
                if (i > 0) line.append(", ");
                line.append(instr.getDefaultOperandRepresentation(i));
                Scalar s = instr.getScalar(i);
                if (s != null && s.getUnsignedValue() > 0 && s.getUnsignedValue() < 0x10000000L)
                    line.append(String.format(" (%d)", s.getUnsignedValue()));
            }
            // Mark calls
            for (Reference ref : instr.getReferencesFrom()) {
                if (ref.getReferenceType().isCall()) {
                    Function cf = fm.getFunctionAt(ref.getToAddress());
                    line.append(" → CALL " + (cf != null ? cf.getName() : ref.getToAddress()));
                }
            }
            println(line.toString());
            count++;
        }
        if (count >= 60) println("  ... (truncated)");
    }

    String toBinary(int val, int bits) {
        StringBuilder sb = new StringBuilder();
        for (int i = bits-1; i >= 0; i--) sb.append((val >> i) & 1);
        return sb.toString();
    }
}
