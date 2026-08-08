// Dump FUN_2000e5d8 (channel setter) and FUN_2000e614 to decode ADC channel.
//@category Analysis

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.*;
import java.util.*;

public class FinalADCDecode extends GhidraScript {

    @Override
    public void run() throws Exception {
        Listing listing = currentProgram.getListing();
        AddressFactory af = currentProgram.getAddressFactory();
        AddressSpace sp = af.getDefaultAddressSpace();
        FunctionManager fm = currentProgram.getFunctionManager();
        Memory mem = currentProgram.getMemory();
        Address anaWriteAddr = sp.getAddress(0x2000047cL);
        Address anaReadAddr  = sp.getAddress(0x2000043cL);

        // FUN_2000e964 at 0x2000e9f8: tadds r0, r4 → r0 = original channel arg
        // then: tjl 0x2000e5d8 → FUN_2000e5d8(channel_arg)
        // FUN_2000e5d8 sets the ADC input channel register
        long[] fnAddrs = {
            0x2000e5d8L,   // channel setter called from FUN_2000e964
            0x2000e614L,   // called from multiple places with channel type args
            0x2000e564L,   // ADC reset/clear function
            0x2000e824L,   // clears 0xe8/e9/ea to 0x00
            0x2000c3dcL,   // called from FUN_2000d430 after ADC
            0x2000e4ccL,   // caller of FUN_2000d430 (what is this?)
            0x2000c4acL,   // caller of FUN_2000c41c
        };
        String[] fnNames = {
            "chan_setter", "chan_type_setter", "ADC_reset",
            "ADC_clear_channels", "post_ADC_d430", "parent_d430", "parent_c41c"
        };

        for (int fi = 0; fi < fnAddrs.length; fi++) {
            Function fn = fm.getFunctionAt(sp.getAddress(fnAddrs[fi]));
            if (fn == null) { println("NOT FOUND: " + fnNames[fi]); continue; }
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
            List<Instruction> w = new ArrayList<>();
            int cnt = 0;
            while (it.hasNext() && cnt < 60) {
                Instruction ins = it.next();
                cnt++;
                w.add(ins);
                if (w.size() > 8) w.remove(0);

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
                            // Extract args
                            int ar0 = -1, ar1 = -1;
                            for (int i = w.size()-2; i >= 0 && (ar0<0 || ar1<0); i--) {
                                Instruction pi = w.get(i);
                                String m = pi.getMnemonicString().toLowerCase();
                                if (!m.contains("mov") && !m.contains("tmovs")) continue;
                                String op0 = pi.getNumOperands()>0 ? pi.getDefaultOperandRepresentation(0) : "";
                                for (int j = 0; j < pi.getNumOperands(); j++) {
                                    Scalar sc = pi.getScalar(j);
                                    if (sc == null) continue;
                                    if (op0.equals("r0") && ar0<0) ar0=(int)sc.getUnsignedValue();
                                    if (op0.equals("r1") && ar1<0) ar1=(int)sc.getUnsignedValue();
                                }
                            }
                            String fn_t = ref.getToAddress().equals(anaWriteAddr) ? "analog_write" : "analog_read";
                            if (ar0 >= 0) {
                                sb.append(" → " + fn_t + "(0x" + Integer.toHexString(ar0) +
                                    (ar1>=0 ? ", 0x"+Integer.toHexString(ar1) : ", ?") + ")");
                                if (ar0 >= 0xe8 && ar0 <= 0xea && ar1 >= 0) {
                                    sb.append(" *** CHANNEL: p=" + ch((ar1>>4)&0xf) + " n=" + ch(ar1&0xf));
                                }
                            } else {
                                sb.append(" → " + fn_t + "(r0, r1)");
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
            if (cnt >= 60) println("  ...(truncated)");
        }

        // Also dump FUN_2000e9f8 area from FUN_2000e964 where channel arg is passed
        println("\n=== FUN_2000e964 instructions around channel arg pass (0x2000e9ce-0x2000ea20) ===");
        Address s2 = sp.getAddress(0x2000e9ceL);
        Address e2 = sp.getAddress(0x2000ea20L);
        InstructionIterator iter = listing.getInstructions(s2, true);
        while (iter.hasNext()) {
            Instruction ins = iter.next();
            if (ins.getAddress().compareTo(e2) >= 0) break;
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
    }

    String ch(int c) {
        switch(c) {
            case 0x0: return "None";
            case 0x1: return "PA6";
            case 0x2: return "PA7";
            case 0x3: return "PB0"; case 0x4: return "PB1"; case 0x5: return "PB2";
            case 0x6: return "PB3"; case 0x7: return "PB4"; case 0x8: return "PB5";
            case 0x9: return "PB6"; case 0xa: return "PB7";
            case 0xe: return "Temp"; case 0xf: return "VBAT";
            default: return "0x"+Integer.toHexString(c);
        }
    }
}
