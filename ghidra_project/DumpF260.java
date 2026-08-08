// Dump FUN_2000f260 and FUN_2000f29c - called from channel setter with GPIO pin arg.
// These functions likely write the actual ADC channel to afe_0xe8.
//@category Analysis

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.*;
import java.util.*;

public class DumpF260 extends GhidraScript {

    Address anaWriteAddr, anaReadAddr;
    FunctionManager fm;
    Listing listing;

    @Override
    public void run() throws Exception {
        listing = currentProgram.getListing();
        AddressFactory af = currentProgram.getAddressFactory();
        AddressSpace sp = af.getDefaultAddressSpace();
        fm = currentProgram.getFunctionManager();
        Memory mem = currentProgram.getMemory();
        anaWriteAddr = sp.getAddress(0x2000047cL);
        anaReadAddr  = sp.getAddress(0x2000043cL);

        // Read literal pool of FUN_2000e5d8: what is at 0x2000e60c?
        println("=== Literal pool of FUN_2000e5d8 ===");
        Address lpAddr = sp.getAddress(0x2000e608L);
        for (int i = 0; i < 4; i++) {
            long val = mem.getInt(lpAddr, false) & 0xFFFFFFFFL;
            println(String.format("  [0x%08x] = 0x%08x", lpAddr.getOffset(), val));
            lpAddr = lpAddr.add(4);
        }

        // Dump FUN_2000f260 (first callee of FUN_2000e5d8 - GPIO pin setup for ADC)
        long[] fnAddrs = {
            0x2000f260L,
            0x2000f29cL,
            0x2000e6b8L,  // continuation of FUN_2000e614 after bit2 branch
        };
        String[] fnNames = {"gpio_adc_pin_setup", "gpio_pull_setup", "e614_continuation"};

        for (int fi = 0; fi < fnAddrs.length; fi++) {
            Function fn = fm.getFunctionAt(sp.getAddress(fnAddrs[fi]));
            if (fn == null) {
                println("NOT FOUND: " + fnNames[fi] + " @ 0x" + Long.toHexString(fnAddrs[fi]));
                // Dump raw instructions anyway
                println("  (raw dump from address)");
                Address rawStart = sp.getAddress(fnAddrs[fi]);
                InstructionIterator it = listing.getInstructions(rawStart, true);
                int c = 0;
                while (it.hasNext() && c++ < 40) {
                    dumpInstr(it.next());
                }
                continue;
            }
            println("\n=== " + fnNames[fi] + " = " + fn.getName() +
                    " @ " + fn.getEntryPoint() + " (size=" + fn.getBody().getNumAddresses() + ") ===");
            ReferenceManager rm = currentProgram.getReferenceManager();
            Set<String> callers = new LinkedHashSet<>();
            for (Reference ref : rm.getReferencesTo(fn.getEntryPoint())) {
                if (ref.getReferenceType().isCall()) {
                    Function c = fm.getFunctionContaining(ref.getFromAddress());
                    callers.add(c != null ? c.getName() + "@" + c.getEntryPoint() : "?");
                }
            }
            println("  Callers: " + callers);

            InstructionIterator it = listing.getInstructions(fn.getBody(), true);
            List<Instruction> window = new ArrayList<>();
            int cnt = 0;
            while (it.hasNext() && cnt < 80) {
                Instruction ins = it.next();
                cnt++;
                window.add(ins);
                if (window.size() > 10) window.remove(0);
                dumpInstrWithAnaAnnotation(ins, window);
            }
            if (cnt >= 80) println("  ...(truncated)");
        }

        // Also dump the tail of FUN_2000e614 to see what the full function does
        println("\n=== FUN_2000e614 full dump (first 80 instrs past 0x2000e69a) ===");
        Address s = sp.getAddress(0x2000e69aL);
        InstructionIterator iter = listing.getInstructions(s, true);
        int c = 0;
        List<Instruction> win = new ArrayList<>();
        while (iter.hasNext() && c++ < 80) {
            Instruction ins = iter.next();
            win.add(ins);
            if (win.size() > 10) win.remove(0);
            dumpInstrWithAnaAnnotation(ins, win);
        }

        // And dump FUN_2000e4ccL (caller of FUN_2000d430, which calls FUN_2000e964(0x102))
        println("\n=== FUN_2000e4cc (caller of FUN_2000d430 which is ADC caller) ===");
        Function fn_e4cc = fm.getFunctionAt(sp.getAddress(0x2000e4ccL));
        if (fn_e4cc != null) {
            println("  size=" + fn_e4cc.getBody().getNumAddresses());
            ReferenceManager rm2 = currentProgram.getReferenceManager();
            Set<String> callers2 = new LinkedHashSet<>();
            for (Reference ref : rm2.getReferencesTo(fn_e4cc.getEntryPoint())) {
                if (ref.getReferenceType().isCall()) {
                    Function cc = fm.getFunctionContaining(ref.getFromAddress());
                    callers2.add(cc != null ? cc.getName() + "@" + cc.getEntryPoint() : "?");
                }
            }
            println("  Callers: " + callers2);
            InstructionIterator it2 = listing.getInstructions(fn_e4cc.getBody(), true);
            List<Instruction> win2 = new ArrayList<>();
            int cnt2 = 0;
            while (it2.hasNext() && cnt2++ < 60) {
                Instruction ins = it2.next();
                win2.add(ins); if (win2.size() > 10) win2.remove(0);
                dumpInstrWithAnaAnnotation(ins, win2);
            }
        }

        // FUN_2000c4ac (caller of FUN_2000c41c battery ADC)
        println("\n=== FUN_2000c4ac (caller of battery ADC FUN_2000c41c) ===");
        Function fn_c4ac = fm.getFunctionAt(sp.getAddress(0x2000c4acL));
        if (fn_c4ac != null) {
            println("  size=" + fn_c4ac.getBody().getNumAddresses());
            ReferenceManager rm3 = currentProgram.getReferenceManager();
            Set<String> callers3 = new LinkedHashSet<>();
            for (Reference ref : rm3.getReferencesTo(fn_c4ac.getEntryPoint())) {
                if (ref.getReferenceType().isCall()) {
                    Function cc = fm.getFunctionContaining(ref.getFromAddress());
                    callers3.add(cc != null ? cc.getName() + "@" + cc.getEntryPoint() : "?");
                }
            }
            println("  Callers: " + callers3);
            InstructionIterator it3 = listing.getInstructions(fn_c4ac.getBody(), true);
            List<Instruction> win3 = new ArrayList<>();
            int cnt3 = 0;
            while (it3.hasNext() && cnt3++ < 60) {
                Instruction ins = it3.next();
                win3.add(ins); if (win3.size() > 10) win3.remove(0);
                dumpInstrWithAnaAnnotation(ins, win3);
            }
        }
    }

    void dumpInstr(Instruction ins) {
        StringBuilder sb = new StringBuilder();
        sb.append(String.format("  0x%08x: %-14s", ins.getAddress().getOffset(),
            ins.getMnemonicString()));
        for (int i = 0; i < ins.getNumOperands(); i++) {
            if (i > 0) sb.append(", ");
            sb.append(ins.getDefaultOperandRepresentation(i));
            Scalar s = ins.getScalar(i);
            if (s != null && s.getUnsignedValue() > 0 && s.getUnsignedValue() < 0x10000000L)
                sb.append(String.format("(=0x%x)", s.getUnsignedValue()));
        }
        for (Reference ref : ins.getReferencesFrom()) {
            if (ref.getReferenceType().isCall()) {
                Function cf = fm.getFunctionAt(ref.getToAddress());
                sb.append(" → CALL " + (cf != null ? cf.getName() : ref.getToAddress()));
            } else if (ref.getReferenceType().isData()) {
                sb.append(String.format(" → [0x%08x]", ref.getToAddress().getOffset()));
            }
        }
        println(sb.toString());
    }

    void dumpInstrWithAnaAnnotation(Instruction ins, List<Instruction> window) {
        StringBuilder sb = new StringBuilder();
        sb.append(String.format("  0x%08x: %-14s", ins.getAddress().getOffset(),
            ins.getMnemonicString()));
        for (int i = 0; i < ins.getNumOperands(); i++) {
            if (i > 0) sb.append(", ");
            sb.append(ins.getDefaultOperandRepresentation(i));
            Scalar s = ins.getScalar(i);
            if (s != null && s.getUnsignedValue() > 0 && s.getUnsignedValue() < 0x10000000L)
                sb.append(String.format("(=0x%x)", s.getUnsignedValue()));
        }
        for (Reference ref : ins.getReferencesFrom()) {
            if (ref.getReferenceType().isCall()) {
                Function cf = fm.getFunctionAt(ref.getToAddress());
                if (ref.getToAddress().equals(anaWriteAddr) || ref.getToAddress().equals(anaReadAddr)) {
                    int ar0 = -1, ar1 = -1;
                    for (int i = window.size()-2; i >= 0 && (ar0<0||ar1<0); i--) {
                        Instruction pi = window.get(i);
                        String m = pi.getMnemonicString().toLowerCase();
                        if (!m.contains("mov") && !m.contains("tmovs") && !m.contains("tadds")) continue;
                        String op0 = pi.getNumOperands()>0 ? pi.getDefaultOperandRepresentation(0):"";
                        Scalar sc = null;
                        for (int j = 0; j < pi.getNumOperands(); j++) { sc=pi.getScalar(j); if(sc!=null)break; }
                        if (sc == null) continue;
                        if (op0.equals("r0") && ar0<0) ar0=(int)sc.getUnsignedValue();
                        if (op0.equals("r1") && ar1<0) ar1=(int)sc.getUnsignedValue();
                    }
                    String fnT = ref.getToAddress().equals(anaWriteAddr) ? "analog_write" : "analog_read";
                    if (ar0 >= 0) {
                        sb.append(" → " + fnT + "(0x" + Integer.toHexString(ar0) +
                            (ar1>=0 ? ", 0x"+Integer.toHexString(ar1) : ", ?") + ")");
                        if (ar0 >= 0xe8 && ar0 <= 0xea) {
                            int p = (ar1 >> 4) & 0xf, n = ar1 & 0xf;
                            sb.append(" *** ADC CHANNEL: p=" + ch(p) + " n=" + ch(n));
                        }
                    } else {
                        sb.append(" → " + fnT + "(r0,r1)");
                    }
                } else {
                    sb.append(" → CALL " + (cf != null ? cf.getName() : ref.getToAddress()));
                }
            } else if (ref.getReferenceType().isData()) {
                sb.append(String.format(" → [0x%08x]", ref.getToAddress().getOffset()));
            }
        }
        println(sb.toString());
    }

    String ch(int c) {
        switch(c) {
            case 0x0: return "None"; case 0x1: return "PA6"; case 0x2: return "PA7";
            case 0x3: return "PB0"; case 0x4: return "PB1"; case 0x5: return "PB2";
            case 0x6: return "PB3"; case 0x7: return "PB4"; case 0x8: return "PB5";
            case 0x9: return "PB6"; case 0xa: return "PB7";
            case 0xe: return "Temp"; case 0xf: return "VBAT";
            default: return "0x"+Integer.toHexString(c);
        }
    }
}
