// Finds the battery ADC function and traces the analog pin configuration.
//@category Analysis

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.*;
import java.util.*;

public class BatteryPinTrace extends GhidraScript {

    @Override
    public void run() throws Exception {
        Listing listing = currentProgram.getListing();
        AddressFactory af = currentProgram.getAddressFactory();
        AddressSpace sp = af.getDefaultAddressSpace();
        FunctionManager fm = currentProgram.getFunctionManager();
        Memory mem = currentProgram.getMemory();

        println("=== Battery ADC Pin Tracer ===\n");

        // Step 1: Find the battery measurement function.
        // The primary candidate is the function containing 3500mV at 0x20003f63.
        Address ref3500 = sp.getAddress(0x20003f63L);
        Function battFn = fm.getFunctionContaining(ref3500);
        if (battFn == null) {
            // Try nearby addresses
            for (int delta = -4; delta <= 4; delta += 2) {
                battFn = fm.getFunctionContaining(sp.getAddress(0x20003f63L + delta));
                if (battFn != null) break;
            }
        }
        println("Battery function (contains 3500mV literal): " +
            (battFn != null ? battFn.getName() + " @ " + battFn.getEntryPoint() : "not found"));

        // Step 2: Also look at FUN_2000802c (uses 70,90 thresholds) as candidate
        Address fn802c = sp.getAddress(0x2000802cL);
        Function battFn2 = fm.getFunctionContaining(fn802c);
        println("Battery percent function (contains 70/90): " +
            (battFn2 != null ? battFn2.getName() + " @ " + battFn2.getEntryPoint() : "not found"));

        // Step 3: Dump all scalar/immediate values in the 3500mV-containing function
        if (battFn != null) {
            println("\nAll scalars in battery function " + battFn.getName() + ":");
            InstructionIterator iter = listing.getInstructions(battFn.getBody(), true);
            while (iter.hasNext()) {
                Instruction instr = iter.next();
                for (int i = 0; i < instr.getNumOperands(); i++) {
                    Scalar s = instr.getScalar(i);
                    if (s != null && s.getUnsignedValue() > 0) {
                        long v = s.getUnsignedValue();
                        println(String.format("  0x%08x: %s  -> scalar=%d (0x%x)",
                            instr.getAddress().getOffset(),
                            instr.getMnemonicString(), v, v));
                    }
                }
            }
        }

        // Step 4: Search all flash for 16-bit voltage thresholds that could be battery mV ranges
        // These appear as literal pool entries: search in LE16 format
        println("\nAll 16-bit voltage literals (3300-4300 mV) in flash:");
        byte[] flash = new byte[0x40000];
        mem.getBytes(sp.getAddress(0x20000000L), flash);
        for (int off = 0; off < flash.length - 1; off += 2) {
            int val = (flash[off] & 0xff) | ((flash[off+1] & 0xff) << 8);
            if (val >= 3300 && val <= 4300) {
                Address a = sp.getAddress(0x20000000L + off);
                Function f = fm.getFunctionContaining(a);
                // Only report if in a function or close to one
                String fnName = f != null ? f.getName() + "@" + f.getEntryPoint() : "(literal pool)";
                println(String.format("  0x%08x: %d mV (0x%04x) in %s",
                    0x20000000 + off, val, val, fnName));
            }
        }

        // Step 5: Search flash for ADC-range raw values
        // TLSR8232 ADC: Vref ~1.2V, 14-bit -> 1LSB = 1.2/16384 V
        // Battery 3.5V -> 3500/1200 * 16384 = 47787 raw (way out of 14-bit range)
        // So firmware likely uses 10-bit mode: 1LSB = 1.2/1024 = 1.17mV
        // 3500mV / 1.17mV ≈ 2991 raw
        // Or uses supply/4 input: 4200mV/4 = 1050mV -> 1050/1200*1024 ≈ 896 raw
        // Common TLSR8232 VBAT raw ADC range: 0x200-0x500
        println("\nSearching for possible ADC raw value thresholds (0x200-0x600 as 16-bit LE):");
        for (int off = 0; off < flash.length - 1; off += 2) {
            int val = (flash[off] & 0xff) | ((flash[off+1] & 0xff) << 8);
            if (val >= 0x200 && val <= 0x600 && (val & 0xF) == 0) { // 16-aligned raw values
                Address a = sp.getAddress(0x20000000L + off);
                Function f = fm.getFunctionContaining(a);
                String fnName = f != null ? f.getName() + "@" + f.getEntryPoint() : "(literal pool)";
                // Only report if in the main code area (below OTA at 0x40000)
                if (off < 0x40000) {
                    println(String.format("  0x%08x: raw=0x%04x (%d) in %s",
                        0x20000000 + off, val, val, fnName));
                }
            }
        }

        // Step 6: Find which function is called most often and has short body
        // (battery read functions are small, frequently called)
        println("\nSmall functions called from battery-percent function:");
        if (battFn2 != null) {
            InstructionIterator iter = listing.getInstructions(battFn2.getBody(), true);
            Set<Address> callTargets = new LinkedHashSet<>();
            while (iter.hasNext()) {
                Instruction instr = iter.next();
                if (instr.getMnemonicString().toLowerCase().contains("call") ||
                    instr.getMnemonicString().toLowerCase().contains("tjl") ||
                    instr.getMnemonicString().toLowerCase().contains("tbl")) {
                    for (Reference ref : instr.getReferencesFrom()) {
                        if (ref.getReferenceType().isCall()) {
                            callTargets.add(ref.getToAddress());
                        }
                    }
                }
            }
            for (Address ct : callTargets) {
                Function cf = fm.getFunctionAt(ct);
                if (cf != null) {
                    println("  calls: " + cf.getName() + " @ " + ct);
                }
            }
        }
    }
}
