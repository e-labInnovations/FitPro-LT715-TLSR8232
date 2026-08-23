// Finds GPIO register accesses in a raw TC32 image.
//
// The register space is not part of the image, so the code reaches it through
// literal-pool constants: `ldr rX, [pc, #imm]` loads 0x800583 from a data word,
// then stores through it. Matching decompiled text for "0x800583" therefore
// finds nothing - the decompiler prints the literal's symbol instead. So: find
// the literals by value, label them, follow references back to code, and print
// the disassembly around each one, where the bit mask is visible as an immediate.
//@category Analysis
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.symbol.*;
import ghidra.util.task.ConsoleTaskMonitor;
import java.util.*;

public class GpioRefs extends GhidraScript {

    static final LinkedHashMap<Long, String> REGS = new LinkedHashMap<>();
    static {
        String[] r = {"IN","IE","OEN","OUT","POL","DS","FUNC","IRQ"};
        String[] p = {"PA","PB","PC"};
        long[] base = {0x800580L, 0x800588L, 0x800590L};
        for (int i = 0; i < p.length; i++)
            for (int j = 0; j < r.length; j++)
                REGS.put(base[i] + j, p[i] + "_" + r[j]);
    }

    public void run() throws Exception {
        Memory mem = currentProgram.getMemory();
        Listing listing = currentProgram.getListing();
        AddressSpace sp = currentProgram.getAddressFactory().getDefaultAddressSpace();
        long lo = 0, hi = 0x1a8a4;

        // 1. locate literal-pool words holding a GPIO register address
        Map<Address, String> lits = new LinkedHashMap<>();
        for (long a = lo; a + 4 <= hi; a += 2) {          // TC32 literals are 4-aligned but scan by 2
            Address addr = sp.getAddress(a);
            long v;
            try { v = mem.getInt(addr) & 0xffffffffL; } catch (Exception e) { continue; }
            String name = REGS.get(v);
            if (name != null) lits.put(addr, name);
        }
        println("### literal words holding GPIO register addresses: " + lits.size());
        for (Map.Entry<Address, String> e : lits.entrySet())
            println("   " + e.getKey() + "  " + e.getValue());

        // 2. references back into code, grouped by function
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        Map<Function, Set<String>> byFunc = new LinkedHashMap<>();
        int refs = 0;
        for (Map.Entry<Address, String> e : lits.entrySet()) {
            for (Reference ref : getReferencesTo(e.getKey())) {
                Address from = ref.getFromAddress();
                Function f = getFunctionContaining(from);
                refs++;
                if (f == null) { println("   ref from " + from + " (" + e.getValue() + ") - no function"); continue; }
                byFunc.computeIfAbsent(f, k -> new LinkedHashSet<>()).add(e.getValue());
            }
        }
        println("### references: " + refs + " from " + byFunc.size() + " function(s)");

        // 3. for each function: disassembly with the immediates, then the C
        int dumped = 0;
        for (Map.Entry<Function, Set<String>> e : byFunc.entrySet()) {
            Function f = e.getKey();
            println("");
            println("======== " + f.getName() + " @ " + f.getEntryPoint()
                    + "  size=" + f.getBody().getNumAddresses() + "  regs=" + e.getValue());
            if (dumped++ < 8) {
                InstructionIterator it = listing.getInstructions(f.getBody(), true);
                while (it.hasNext()) {
                    Instruction ins = it.next();
                    println("  " + ins.getAddress() + "  " + ins);
                }
                DecompileResults res = di.decompileFunction(f, 45, new ConsoleTaskMonitor());
                if (res != null && res.decompileCompleted()) {
                    println("  --- decompiled ---");
                    for (String line : res.getDecompiledFunction().getC().split("\n"))
                        println("  " + line);
                }
            }
        }
        di.dispose();
        println("### done");
    }
}
