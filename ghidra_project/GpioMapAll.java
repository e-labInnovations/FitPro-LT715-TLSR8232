// Table of every GPIO register access in the image, with the pin bits.
//
// The register space is not in the image, so the code reaches it through
// literal-pool words: `ldr rX,[pc,#imm]` loads 0x800583, then stores through it.
// Grepping decompiled text for the address finds nothing - the decompiler prints
// the literal's symbol. So: find literals by value, follow references to code,
// then read the mask immediates near each reference, which is where the pin
// number actually lives.
//
// Output per function: the registers it touches and the bit masks it applies,
// e.g. "FUN_0000cfc8  PA_OUT[bit5]" - one line per function, so a single pass
// gives the whole pin map instead of one pin at a time.
//@category Analysis
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.symbol.*;
import java.util.*;

public class GpioMapAll extends GhidraScript {

    static final LinkedHashMap<Long, String> REGS = new LinkedHashMap<>();
    static {
        String[] r = {"IN","IE","OEN","OUT","POL","DS","FUNC","IRQ"};
        String[] p = {"PA","PB","PC"};
        long[] base = {0x800580L, 0x800588L, 0x800590L};
        for (int i = 0; i < p.length; i++)
            for (int j = 0; j < r.length; j++)
                REGS.put(base[i] + j, p[i] + "_" + r[j]);
    }

    // window of instructions after the literal load in which a mask counts
    static final int AFTER = 8;

    static String bits(int mask) {
        // a mask and its complement name the same pin: 0x20 sets bit5, 0xdf clears it
        int m = mask & 0xff;
        int eff = (Integer.bitCount(m) > 4) ? (~m & 0xff) : m;
        StringBuilder sb = new StringBuilder();
        for (int b = 0; b < 8; b++)
            if ((eff & (1 << b)) != 0) { if (sb.length() > 0) sb.append(','); sb.append(b); }
        return sb.length() == 0 ? "-" : sb.toString();
    }

    public void run() throws Exception {
        Memory mem = currentProgram.getMemory();
        Listing listing = currentProgram.getListing();
        AddressSpace sp = currentProgram.getAddressFactory().getDefaultAddressSpace();

        Map<Address, String> lits = new LinkedHashMap<>();
        for (MemoryBlock blk : mem.getBlocks()) {
            if (!blk.isInitialized()) continue;
            for (long a = blk.getStart().getOffset(); a + 4 <= blk.getEnd().getOffset(); a += 2) {
                Address addr = sp.getAddress(a);
                long v;
                try { v = mem.getInt(addr) & 0xffffffffL; } catch (Exception e) { continue; }
                String name = REGS.get(v);
                if (name != null) lits.put(addr, name);
            }
        }
        println("### GPIO register literals: " + lits.size());

        // function -> "REG[bits]" -> count
        Map<String, Map<String, Integer>> table = new TreeMap<>();
        int refs = 0;
        for (Map.Entry<Address, String> e : lits.entrySet()) {
            for (Reference ref : getReferencesTo(e.getKey())) {
                Address from = ref.getFromAddress();
                Function f = getFunctionContaining(from);
                String fname = (f == null ? "<none>@" + from : f.getName() + "@" + f.getEntryPoint());
                refs++;

                Set<Integer> masks = new LinkedHashSet<>();
                Instruction ins = listing.getInstructionAt(from);
                for (int i = 0; i < AFTER && ins != null; i++, ins = ins.getNext()) {
                    String t = ins.getMnemonicString();
                    if (!t.startsWith("tmovs") && !t.startsWith("tand") && !t.startsWith("tor")
                        && !t.startsWith("tbclr") && !t.startsWith("tbset")) continue;
                    for (int op = 0; op < ins.getNumOperands(); op++) {
                        Object[] objs = ins.getOpObjects(op);
                        for (Object o : objs)
                            if (o instanceof ghidra.program.model.scalar.Scalar)
                                masks.add((int) ((ghidra.program.model.scalar.Scalar) o).getValue());
                    }
                }
                String key;
                if (masks.isEmpty()) key = e.getValue() + "[?]";
                else {
                    StringBuilder sb = new StringBuilder(e.getValue()).append("[bit");
                    Set<String> seen = new LinkedHashSet<>();
                    for (int m : masks) if (m > 0 && m < 256) seen.add(bits(m));
                    sb.append(String.join("/", seen)).append("]");
                    key = sb.toString();
                }
                table.computeIfAbsent(fname, k -> new TreeMap<>()).merge(key, 1, Integer::sum);
            }
        }

        println("### references: " + refs + " across " + table.size() + " functions");
        for (Map.Entry<String, Map<String, Integer>> e : table.entrySet())
            println("   " + e.getKey() + "   " + e.getValue());
        println("### done");
    }
}
