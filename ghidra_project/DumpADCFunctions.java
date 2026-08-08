// Dumps the ADC configuration functions and traces battery read path.
//@category Analysis

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.*;
import java.util.*;

public class DumpADCFunctions extends GhidraScript {

    @Override
    public void run() throws Exception {
        Listing listing = currentProgram.getListing();
        AddressFactory af = currentProgram.getAddressFactory();
        AddressSpace sp = af.getDefaultAddressSpace();
        FunctionManager fm = currentProgram.getFunctionManager();
        Memory mem = currentProgram.getMemory();

        // Key ADC functions found from analog_write analysis:
        long[] adcFnAddrs = {
            0x2000e964L,  // writes analog 0xef (ADC MODE)
            0x2000fb20L,  // writes analog 0x2f
            0x2000fbf4L,  // writes analog 0x2d (channel select, 0xe8 and 0x68)
            0x2000ee70L,  // called from FUN_20003f28 (battery related fn)
            0x2000ff78L,  // second ADC MODE writer (analog 0xef, 0xf1, 0xf2, 0xf9, 0xfc)
            0x2000ea74L,  // called from many ADC chain fns (writes 0xfc)
            0x2000ebf0L,  // writes 0xfc
        };

        for (long addr : adcFnAddrs) {
            Function fn = fm.getFunctionAt(sp.getAddress(addr));
            if (fn == null) {
                for (int d = -4; d <= 4; d += 2) {
                    fn = fm.getFunctionAt(sp.getAddress(addr + d));
                    if (fn != null) break;
                }
            }
            if (fn == null) { println("NOT FOUND: 0x" + Long.toHexString(addr)); continue; }

            println("\n=== " + fn.getName() + " @ " + fn.getEntryPoint() +
                    " (size=" + fn.getBody().getNumAddresses() + ") ===");

            // Show callers
            Set<String> callers = new LinkedHashSet<>();
            ReferenceManager rm = currentProgram.getReferenceManager();
            for (Reference ref : rm.getReferencesTo(fn.getEntryPoint())) {
                if (ref.getReferenceType().isCall()) {
                    Function caller = fm.getFunctionContaining(ref.getFromAddress());
                    callers.add(caller != null ?
                        caller.getName() + "@" + caller.getEntryPoint() :
                        "0x" + Long.toHexString(ref.getFromAddress().getOffset()));
                }
            }
            println("  Callers: " + callers);

            // Dump instructions
            InstructionIterator iter = listing.getInstructions(fn.getBody(), true);
            int cnt = 0;
            while (iter.hasNext() && cnt++ < 80) {
                Instruction instr = iter.next();
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
                        sb.append(String.format(" → [0x%08x]", ref.getToAddress().getOffset()));
                    }
                }
                println(sb.toString());
            }
            if (cnt >= 80) println("  ...(truncated)");
        }

        // === ADC channel decode: look for analog_write(0x2d, X) calls ===
        println("\n=== Detailed: All analog_write(0x2d, ?) calls (ADC channel select) ===");
        println("TLSR8232 ana_0x2d: single-ended input select");
        println("Known channels: 0x0B=PB7, 0x0C=PB6, 0x05=VBAT(internal), 0x08=PA4?");

        // === Look for battery ADC callers more broadly ===
        // FUN_2000ee70 is called from FUN_20003f28 - find all callers of FUN_2000ee70
        println("\n=== All callers of FUN_2000ee70 ===");
        Function fn_ee70 = fm.getFunctionAt(sp.getAddress(0x2000ee70L));
        if (fn_ee70 != null) {
            ReferenceManager rm = currentProgram.getReferenceManager();
            for (Reference ref : rm.getReferencesTo(fn_ee70.getEntryPoint())) {
                if (ref.getReferenceType().isCall()) {
                    Function caller = fm.getFunctionContaining(ref.getFromAddress());
                    println("  " + (caller != null ? caller.getName() + "@" + caller.getEntryPoint() : "?") +
                        " calls from 0x" + Long.toHexString(ref.getFromAddress().getOffset()));
                }
            }
        }

        // === Decode what analog_write(0x2d, val) means for channel ===
        println("\n=== ADC channel interpretation ===");
        println("FUN_2000fbf4 calls: analog_write(0x2d, 0xe8) then analog_write(0x2d, 0x68)");
        println("0xe8 = 11101000b");
        println("0x68 = 01101000b");
        println("Lower 5 bits: 01000 = 8 (channel 8 in some TLSR8232 channel maps)");
        println("Bit difference: bit7=1 (0xe8) vs bit7=0 (0x68)");
        println("In TLSR8232 ADC:");
        println("  Ana_reg 0x2d bits[4:0] = ADC left channel input select");
        println("  Ana_reg 0x2d bits[6:5] = ADC left channel calibration mode");
        println("  Ana_reg 0x2d bit7 = ADC left channel power enable");
        println("Channels:");
        println("  0x00 = GND/VBAT_DIV/None");
        println("  0x01 = PA0");
        println("  0x02 = PA1");
        println("  0x03 = PA2");
        println("  0x04 = PA3");
        println("  0x05 = PA4");
        println("  0x06 = PA5");
        println("  0x07 = PA6");
        println("  0x08 = PA7");  // <-- Check! 0x68 lower 5 bits = 01000 = 8 → PA7?
        println("  0x09 = PB0");
        println("  0x0a = PB1");
        println("  0x0b = PB2");
        println("  0x0c = PB3");
        println("  0x0d = PB4");
        println("  0x0e = PB5");
        println("  0x0f = PB6");
        println("  0x10 = PB7");
        println("  0x11 = VBAT internal");
        println("Note: actual channel numbering varies by TLSR chip - need to verify against SDK header");

        // Also look for the function FUN_2000ee84 (calls analog_write 0x30 and 0x83)
        println("\n=== FUN_2000ee84 (GPIO init, calls analog_write 0x30) ===");
        Function fn_ee84 = fm.getFunctionAt(sp.getAddress(0x2000ee84L));
        if (fn_ee84 != null) {
            println("Callers:");
            ReferenceManager rm = currentProgram.getReferenceManager();
            for (Reference ref : rm.getReferencesTo(fn_ee84.getEntryPoint())) {
                if (ref.getReferenceType().isCall()) {
                    Function caller = fm.getFunctionContaining(ref.getFromAddress());
                    println("  " + (caller != null ? caller.getName() + "@" + caller.getEntryPoint() : "?"));
                }
            }
            InstructionIterator iter = listing.getInstructions(fn_ee84.getBody(), true);
            int cnt = 0;
            while (iter.hasNext() && cnt++ < 60) {
                Instruction instr = iter.next();
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
                        sb.append(String.format(" → [0x%08x]", ref.getToAddress().getOffset()));
                    }
                }
                println(sb.toString());
            }
        }
    }
}
