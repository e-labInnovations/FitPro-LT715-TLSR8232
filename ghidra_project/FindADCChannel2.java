// Find all analog_write calls to ADC channel registers 0xe8/0xe9/0xea and dump callers.
//@category Analysis

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.*;
import java.util.*;

public class FindADCChannel2 extends GhidraScript {

    @Override
    public void run() throws Exception {
        Listing listing = currentProgram.getListing();
        AddressFactory af = currentProgram.getAddressFactory();
        AddressSpace sp = af.getDefaultAddressSpace();
        FunctionManager fm = currentProgram.getFunctionManager();
        Memory mem = currentProgram.getMemory();
        Address anaWriteAddr = sp.getAddress(0x2000047cL);
        Address anaReadAddr  = sp.getAddress(0x2000043cL);

        println("=== All analog_write(0xe8..0xea, X) calls (ADC channel input select) ===");
        println("Datasheet: afe_0xe8 = Misc channel: [7:4]=p_input [3:0]=n_input");
        println("           afe_0xe9 = Left channel:  [7:4]=p_input [3:0]=n_input");
        println("           afe_0xea = Right channel: [7:4]=p_input [3:0]=n_input");
        println("Channel map: 0x0=none 0x1=PA6 0x2=PA7 0x3=PB0 0x4=PB1 ... 0xa=PB7");
        println("             0xe=tempsensor 0xf=Vbat_internal");

        // Walk all instructions, look back up to 20 instructions for r0 assignment
        InstructionIterator iter = listing.getInstructions(true);
        List<Instruction> window = new ArrayList<>();
        while (iter.hasNext()) {
            Instruction instr = iter.next();
            window.add(instr);
            if (window.size() > 20) window.remove(0);

            boolean isAnaWrite = false;
            for (Reference ref : instr.getReferencesFrom()) {
                if (ref.getReferenceType().isCall() && ref.getToAddress().equals(anaWriteAddr)) {
                    isAnaWrite = true; break;
                }
            }
            if (!isAnaWrite) continue;

            // Extract r0 (reg addr) from the last 20 instructions
            int r0 = -1, r1 = -1;
            for (int i = window.size() - 2; i >= 0 && (r0 < 0 || r1 < 0); i--) {
                Instruction pi = window.get(i);
                String m = pi.getMnemonicString().toLowerCase();
                if (!m.contains("mov") && !m.contains("add") && !m.contains("tmovs")) continue;
                String op0 = pi.getNumOperands() > 0 ? pi.getDefaultOperandRepresentation(0) : "";
                Scalar s = null;
                for (int j = 0; j < pi.getNumOperands(); j++) {
                    s = pi.getScalar(j);
                    if (s != null) break;
                }
                if (s == null) continue;
                if (op0.equals("r0") && r0 < 0) r0 = (int) s.getUnsignedValue();
                if (op0.equals("r1") && r1 < 0) r1 = (int) s.getUnsignedValue();
            }

            if (r0 >= 0xe8 && r0 <= 0xea) {
                Function caller = fm.getFunctionContaining(instr.getAddress());
                String channel_p = decodeChannel((r1 >> 4) & 0xf);
                String channel_n = decodeChannel(r1 & 0xf);
                println(String.format(
                    "  0x%08x: analog_write(0x%02x, 0x%02x)  p_input=%s  n_input=%s  in %s",
                    instr.getAddress().getOffset(), r0, (r1 < 0 ? 0 : r1),
                    channel_p, channel_n,
                    caller != null ? caller.getName() + "@" + caller.getEntryPoint() : "?"));
            }
        }

        // === Dump FUN_2000c57c, FUN_2000d430, FUN_2000c41c (callers of ADC init) ===
        println("\n=== ADC init callers: FUN_2000c57c, FUN_2000d430, FUN_2000c41c ===");
        long[] fnAddrs = {0x2000c57cL, 0x2000d430L, 0x2000c41cL};
        for (long addr : fnAddrs) {
            Function fn = fm.getFunctionAt(sp.getAddress(addr));
            if (fn == null) { println("NOT FOUND: 0x" + Long.toHexString(addr)); continue; }
            println("\n-- " + fn.getName() + " @ " + fn.getEntryPoint() +
                    " (size=" + fn.getBody().getNumAddresses() + ") --");
            // Show callers
            Set<String> callers = new LinkedHashSet<>();
            ReferenceManager rm = currentProgram.getReferenceManager();
            for (Reference ref : rm.getReferencesTo(fn.getEntryPoint())) {
                if (ref.getReferenceType().isCall()) {
                    Function c = fm.getFunctionContaining(ref.getFromAddress());
                    callers.add(c != null ? c.getName() + "@" + c.getEntryPoint() : "?");
                }
            }
            println("  Callers: " + callers);
            // Dump first 60 instructions showing all analog_write calls with args
            InstructionIterator it = listing.getInstructions(fn.getBody(), true);
            List<Instruction> w2 = new ArrayList<>();
            int cnt = 0;
            while (it.hasNext() && cnt < 80) {
                Instruction ins = it.next();
                cnt++;
                w2.add(ins);
                if (w2.size() > 10) w2.remove(0);

                boolean isCall = false;
                Address callTarget = null;
                for (Reference ref : ins.getReferencesFrom()) {
                    if (ref.getReferenceType().isCall()) {
                        isCall = true; callTarget = ref.getToAddress(); break;
                    }
                }

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
                if (isCall) {
                    Function cf = fm.getFunctionAt(callTarget);
                    String cfName = cf != null ? cf.getName() : callTarget.toString();
                    // Check if it's analog_write/read
                    if (callTarget.equals(anaWriteAddr) || callTarget.equals(anaReadAddr)) {
                        int ar0 = -1, ar1 = -1;
                        for (int i = w2.size() - 2; i >= 0 && (ar0 < 0 || ar1 < 0); i--) {
                            Instruction pi = w2.get(i);
                            String m = pi.getMnemonicString().toLowerCase();
                            if (!m.contains("mov") && !m.contains("tmovs")) continue;
                            String op0 = pi.getNumOperands() > 0 ? pi.getDefaultOperandRepresentation(0) : "";
                            Scalar sc = null;
                            for (int j = 0; j < pi.getNumOperands(); j++) { sc = pi.getScalar(j); if (sc != null) break; }
                            if (sc == null) continue;
                            if (op0.equals("r0") && ar0 < 0) ar0 = (int) sc.getUnsignedValue();
                            if (op0.equals("r1") && ar1 < 0) ar1 = (int) sc.getUnsignedValue();
                        }
                        String fn_type = callTarget.equals(anaWriteAddr) ? "analog_write" : "analog_read";
                        if (ar0 >= 0) {
                            sb.append(" → " + fn_type + "(0x" + Integer.toHexString(ar0) + ", " +
                                (ar1 < 0 ? "?" : "0x" + Integer.toHexString(ar1)) + ")");
                            if (ar0 >= 0xe8 && ar0 <= 0xea) {
                                String cp = decodeChannel((ar1 >> 4) & 0xf);
                                String cn = decodeChannel(ar1 & 0xf);
                                sb.append("  *** ADC CHAN: p=" + cp + " n=" + cn);
                            }
                        } else {
                            sb.append(" → " + fn_type + "(r0, r1)");
                        }
                    } else {
                        sb.append(" → CALL " + cfName);
                    }
                }
                println(sb.toString());
            }
            if (cnt >= 80) println("  ...(truncated)");
        }
    }

    String decodeChannel(int c) {
        switch (c) {
            case 0x0: return "None";
            case 0x1: return "PA6";
            case 0x2: return "PA7";
            case 0x3: return "PB0";
            case 0x4: return "PB1";
            case 0x5: return "PB2";
            case 0x6: return "PB3";
            case 0x7: return "PB4";
            case 0x8: return "PB5";
            case 0x9: return "PB6";
            case 0xa: return "PB7";
            case 0xb: return "PGA_neg";
            case 0xe: return "TempSensor";
            case 0xf: return "VBAT_internal";
            default:  return "0x" + Integer.toHexString(c);
        }
    }
}
