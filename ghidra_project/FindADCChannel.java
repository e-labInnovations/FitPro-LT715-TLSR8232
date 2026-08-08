// Find ADC channel config: dumps FUN_2000b9e0 and all analog_write calls with ADC-range args.
//@category Analysis

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.*;
import java.util.*;

public class FindADCChannel extends GhidraScript {

    @Override
    public void run() throws Exception {
        Listing listing = currentProgram.getListing();
        AddressFactory af = currentProgram.getAddressFactory();
        AddressSpace sp = af.getDefaultAddressSpace();
        FunctionManager fm = currentProgram.getFunctionManager();
        Memory mem = currentProgram.getMemory();

        // analog_write = FUN_2000047c, analog_read = FUN_2000043c
        Address anaWriteAddr = sp.getAddress(0x2000047cL);
        Address anaReadAddr  = sp.getAddress(0x2000043cL);

        // === 1. Dump FUN_2000b9e0 (first callee of battery display function) ===
        println("=== FUN_2000b9e0 (called from battery display function) ===");
        Function fn = fm.getFunctionAt(sp.getAddress(0x2000b9e0L));
        if (fn == null) {
            // Walk forward/back to find function
            for (int d = -8; d <= 8; d += 2) {
                fn = fm.getFunctionAt(sp.getAddress(0x2000b9e0L + d));
                if (fn != null) break;
            }
        }
        if (fn != null) {
            println("Function: " + fn.getName() + " @ " + fn.getEntryPoint() +
                    "  size=" + fn.getBody().getNumAddresses());
            dumpFn(fn, listing, fm, sp);
        } else {
            println("(not found as function entry)");
            // Dump raw instructions anyway
            InstructionIterator it = listing.getInstructions(sp.getAddress(0x2000b9e0L), true);
            int c = 0;
            while (it.hasNext() && c++ < 80) {
                Instruction ins = it.next();
                printInstr(ins, fm);
            }
        }

        // === 2. Find all analog_write calls and collect their arguments ===
        println("\n=== All analog_write(r0, r1) calls - filter for ADC-related regs ===");
        // Walk all instructions, find tjl to anaWriteAddr, look back 2-4 instrs for tmovs r0 and r1
        InstructionIterator iter = listing.getInstructions(true);
        Instruction prev3 = null, prev2 = null, prev1 = null;
        while (iter.hasNext()) {
            Instruction instr = iter.next();
            // Check if this is a call to analog_write
            boolean isAnaWrite = false;
            for (Reference ref : instr.getReferencesFrom()) {
                if (ref.getReferenceType().isCall() &&
                    ref.getToAddress().equals(anaWriteAddr)) {
                    isAnaWrite = true;
                    break;
                }
            }
            if (isAnaWrite) {
                // Try to extract r0 (reg addr) and r1 (value) from recent movs
                int r0 = -1, r1 = -1;
                for (Instruction pi : new Instruction[]{prev3, prev2, prev1}) {
                    if (pi == null) continue;
                    String m = pi.getMnemonicString().toLowerCase();
                    if (m.contains("mov") || m.contains("tmovs")) {
                        for (int i = 0; i < pi.getNumOperands(); i++) {
                            Scalar s = pi.getScalar(i);
                            if (s == null) continue;
                            String op0 = pi.getDefaultOperandRepresentation(0);
                            if (op0.equals("r0") && r0 < 0) r0 = (int) s.getUnsignedValue();
                            if (op0.equals("r1") && r1 < 0) r1 = (int) s.getUnsignedValue();
                        }
                    }
                }
                Function caller = fm.getFunctionContaining(instr.getAddress());
                // Only print ADC-range registers (0x20-0x30 = ADC regs, 0xb0-0xbf = pull ctrl,
                // 0x07-0x0f = power/clock, 0xef = ADC mode)
                if (r0 >= 0) {
                    String note = "";
                    if (r0 >= 0x20 && r0 <= 0x30) note = " *** ADC CONFIG REG";
                    if (r0 == 0xef) note = " *** ADC MODE REG";
                    if (r0 >= 0x28 && r0 <= 0x2f) note = " *** ADC CHANNEL/REF";
                    println(String.format("  0x%08x: analog_write(0x%02x, 0x%02x) in %s%s",
                        instr.getAddress().getOffset(), r0, (r1 < 0 ? 0 : r1),
                        caller != null ? caller.getName() : "?", note));
                }
            }
            prev3 = prev2; prev2 = prev1; prev1 = instr;
        }

        // === 3. Dump the function containing the 3500mV literal (actual battery calc fn) ===
        println("\n=== Function containing 3500mV literal (@ 0x20003f62) ===");
        Function battFn = null;
        for (long a = 0x20003f62L; a >= 0x20003000L; a -= 2) {
            battFn = fm.getFunctionContaining(sp.getAddress(a));
            if (battFn != null) break;
        }
        if (battFn != null) {
            println("Function: " + battFn.getName() + " @ " + battFn.getEntryPoint() +
                    "  size=" + battFn.getBody().getNumAddresses());
            dumpFn(battFn, listing, fm, sp);
        }

        // === 4. Dump callees of FUN_2000802c (battery display fn) ===
        println("\n=== All callees of FUN_2000802c (battery display fn) ===");
        Function dispFn = fm.getFunctionAt(sp.getAddress(0x2000802cL));
        if (dispFn == null) dispFn = fm.getFunctionContaining(sp.getAddress(0x2000802cL));
        if (dispFn != null) {
            Set<Address> callees = new LinkedHashSet<>();
            InstructionIterator di = listing.getInstructions(dispFn.getBody(), true);
            while (di.hasNext()) {
                Instruction ins = di.next();
                for (Reference ref : ins.getReferencesFrom()) {
                    if (ref.getReferenceType().isCall()) callees.add(ref.getToAddress());
                }
            }
            for (Address ca : callees) {
                Function cf = fm.getFunctionAt(ca);
                println("  callee: " + (cf != null ? cf.getName() : "?") + " @ " + ca);
                if (cf != null) {
                    // dump first 20 instructions of callee
                    InstructionIterator ci = listing.getInstructions(cf.getBody(), true);
                    int cnt = 0;
                    while (ci.hasNext() && cnt++ < 20) printInstr(ci.next(), fm);
                    println("  ---");
                }
            }
        }
    }

    void dumpFn(Function f, Listing listing, FunctionManager fm, AddressSpace sp) {
        InstructionIterator iter = listing.getInstructions(f.getBody(), true);
        int cnt = 0;
        while (iter.hasNext() && cnt++ < 100) printInstr(iter.next(), fm);
        if (cnt >= 100) println("  ...(truncated at 100 instrs)");
    }

    void printInstr(Instruction instr, FunctionManager fm) {
        StringBuilder sb = new StringBuilder();
        sb.append(String.format("  0x%08x: %-14s", instr.getAddress().getOffset(),
            instr.getMnemonicString()));
        for (int i = 0; i < instr.getNumOperands(); i++) {
            if (i > 0) sb.append(", ");
            sb.append(instr.getDefaultOperandRepresentation(i));
            Scalar s = instr.getScalar(i);
            if (s != null) {
                long v = s.getUnsignedValue();
                if (v > 0 && v < 0x10000000L)
                    sb.append(String.format("(=0x%x)", v));
            }
        }
        for (Reference ref : instr.getReferencesFrom()) {
            if (ref.getReferenceType().isCall()) {
                Function cf = fm.getFunctionAt(ref.getToAddress());
                sb.append(" → CALL " + (cf != null ? cf.getName() : ref.getToAddress()));
            } else if (ref.getReferenceType().isData()) {
                long tgt = ref.getToAddress().getOffset();
                sb.append(String.format(" → [0x%08x]", tgt));
            }
        }
        println(sb.toString());
    }
}
