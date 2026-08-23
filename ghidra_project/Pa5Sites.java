// Every code site that touches a PA-port register, with the bit mask visible.
//
// GpioRefs proved PA_OUT bit 5 is the vibrator. This narrows to the mask: for
// each reference to a PA register literal, print a window of disassembly around
// it so the and/orr immediate is readable, and flag the windows whose immediates
// include 0x20 (bit 5) or its complement 0xdf. That distinguishes "set bit 5"
// from "clear bit 5" - i.e. whether the motor is driven active high or active
// low - and shows any neighbouring register writes (drive strength, pull, analog)
// that our own driver is missing.
//@category Analysis
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.symbol.*;
import ghidra.util.task.ConsoleTaskMonitor;
import java.util.*;

public class Pa5Sites extends GhidraScript {

    static final LinkedHashMap<Long, String> REGS = new LinkedHashMap<>();
    static {
        String[] r = {"IN","IE","OEN","OUT","POL","DS","FUNC","IRQ"};
        String[] p = {"PA","PB","PC"};
        long[] base = {0x800580L, 0x800588L, 0x800590L};
        for (int i = 0; i < p.length; i++)
            for (int j = 0; j < r.length; j++)
                REGS.put(base[i] + j, p[i] + "_" + r[j]);
    }

    static final int WINDOW = 12;   // instructions either side of the reference

    public void run() throws Exception {
        Memory mem = currentProgram.getMemory();
        Listing listing = currentProgram.getListing();
        AddressSpace sp = currentProgram.getAddressFactory().getDefaultAddressSpace();

        // The image is mapped at 0x20000000, so scan every initialised block
        // rather than hardcoding file offsets.
        Map<Address, String> lits = new LinkedHashMap<>();
        for (MemoryBlock blk : mem.getBlocks()) {
            if (!blk.isInitialized()) continue;
            long lo = blk.getStart().getOffset(), hi = blk.getEnd().getOffset();
            for (long a = lo; a + 4 <= hi; a += 2) {
                Address addr = sp.getAddress(a);
                long v;
                try { v = mem.getInt(addr) & 0xffffffffL; } catch (Exception e) { continue; }
                String name = REGS.get(v);
                if (name != null && name.startsWith("PA")) lits.put(addr, name);
            }
        }
        println("### PA register literals: " + lits.size());
        for (Map.Entry<Address, String> e : lits.entrySet())
            println("   " + e.getKey() + "  " + e.getValue());

        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        Set<Function> bit5Funcs = new LinkedHashSet<>();

        for (Map.Entry<Address, String> e : lits.entrySet()) {
            for (Reference ref : getReferencesTo(e.getKey())) {
                Address from = ref.getFromAddress();
                Function f = getFunctionContaining(from);
                println("");
                println("---- " + e.getValue() + " referenced from " + from
                        + " in " + (f == null ? "<no function>" : f.getName()));

                // walk back WINDOW instructions, then forward 2*WINDOW
                Instruction ins = listing.getInstructionAt(from);
                if (ins == null) { println("   (no instruction)"); continue; }
                Instruction start = ins;
                for (int i = 0; i < WINDOW && start.getPrevious() != null; i++) start = start.getPrevious();
                Instruction cur = start;
                boolean hit = false;
                for (int i = 0; i < WINDOW * 2 + 1 && cur != null; i++) {
                    String txt = cur.toString();
                    String mark = cur.getAddress().equals(from) ? " <== " : "     ";
                    boolean m = txt.contains("0x20") || txt.contains("0xdf")
                             || txt.contains("#32") || txt.contains("#223");
                    if (m) hit = true;
                    println("   " + mark + cur.getAddress() + "  " + txt + (m ? "   *bit5*" : ""));
                    cur = cur.getNext();
                }
                if (hit && f != null) bit5Funcs.add(f);
            }
        }

        println("");
        println("### functions with a bit-5 immediate near a PA access: " + bit5Funcs.size());
        for (Function f : bit5Funcs) {
            println("");
            println("======== " + f.getName() + " @ " + f.getEntryPoint()
                    + "  size=" + f.getBody().getNumAddresses());
            DecompileResults res = di.decompileFunction(f, 60, new ConsoleTaskMonitor());
            if (res != null && res.decompileCompleted())
                for (String line : res.getDecompiledFunction().getC().split("\n"))
                    println("  " + line);
            println("  --- callers ---");
            for (Function c : f.getCallingFunctions(new ConsoleTaskMonitor()))
                println("    " + c.getName() + " @ " + c.getEntryPoint());
        }
        di.dispose();
        println("### done");
    }
}
