// Turns GPIO register accesses into pin names.
//
// The literal-pool word holding each register address becomes a DAT_ symbol in
// the decompiler's output, so mapping that symbol back to the register lets any
// line like `*DAT_00004fd8 = *DAT_00004fd8 | 2` be read as "PA_OUT bit 1 = PA1".
// Masks in an AND are inverted first: `& 0xfd` clears bit 1.
//@category Analysis
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.symbol.*;
import ghidra.util.task.ConsoleTaskMonitor;
import java.util.*;
import java.util.regex.*;

public class GpioPins extends GhidraScript {

    static final LinkedHashMap<Long, String> REGS = new LinkedHashMap<>();
    static {
        String[] r = {"IN","IE","OEN","OUT","POL","DS","FUNC","IRQ"};
        String[] p = {"PA","PB","PC"};
        long[] base = {0x800580L, 0x800588L, 0x800590L};
        for (int i = 0; i < p.length; i++)
            for (int j = 0; j < r.length; j++)
                REGS.put(base[i] + j, p[i] + "_" + r[j]);
    }
    static final Map<String, String> KNOWN = new HashMap<>();
    static {
        KNOWN.put("PA1","LCD CS");  KNOWN.put("PA6","LCD RST");
        KNOWN.put("PB1","VBAT/4");  KNOWN.put("PB3","backlight");
        KNOWN.put("PB4","UART TX"); KNOWN.put("PB5","UART RX");
        KNOWN.put("PC1","LCD DC");  KNOWN.put("PC2","touch key");
        KNOWN.put("PC3","LCD MOSI");KNOWN.put("PC5","LCD CLK");
        KNOWN.put("PC7","SWS");
    }

    static final Pattern DAT = Pattern.compile("DAT_([0-9a-fA-F]{8})");
    static final Pattern NUM = Pattern.compile("(?:0x([0-9a-fA-F]+)|\\b(\\d+)\\b)");

    public void run() throws Exception {
        Memory mem = currentProgram.getMemory();
        AddressSpace sp = currentProgram.getAddressFactory().getDefaultAddressSpace();

        Map<String, String> litToReg = new HashMap<>();      // "00004fd8" -> "PA_OUT"
        Set<Address> litAddrs = new LinkedHashSet<>();
        for (long a = 0; a + 4 <= 0x1a8a4; a += 2) {
            Address addr = sp.getAddress(a);
            long v;
            try { v = mem.getInt(addr) & 0xffffffffL; } catch (Exception e) { continue; }
            String name = REGS.get(v);
            if (name == null) continue;
            litToReg.put(String.format("%08x", a), name);
            litAddrs.add(addr);
        }

        LinkedHashSet<Function> funcs = new LinkedHashSet<>();
        for (Address la : litAddrs)
            for (Reference ref : getReferencesTo(la)) {
                Function f = getFunctionContaining(ref.getFromAddress());
                if (f != null) funcs.add(f);
            }
        println("### " + litToReg.size() + " register literals, " + funcs.size() + " functions");

        // pin -> what touches it
        TreeMap<String, List<String>> byPin = new TreeMap<>();
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);

        for (Function f : funcs) {
            DecompileResults res = di.decompileFunction(f, 45, new ConsoleTaskMonitor());
            if (res == null || !res.decompileCompleted()) continue;
            for (String line : res.getDecompiledFunction().getC().split("\n")) {
                Matcher dm = DAT.matcher(line);
                String reg = null;
                while (dm.find()) {
                    String r = litToReg.get(dm.group(1).toLowerCase());
                    if (r != null) { reg = r; break; }
                }
                if (reg == null) continue;
                String port = reg.substring(0, 2), kind = reg.substring(3);
                boolean isAnd = line.contains("&");

                List<Integer> nums = new ArrayList<>();
                Matcher nm = NUM.matcher(line);
                while (nm.find()) {
                    try {
                        nums.add(nm.group(1) != null ? Integer.parseInt(nm.group(1), 16)
                                                     : Integer.parseInt(nm.group(2)));
                    } catch (Exception ignored) {}
                }
                for (int raw : nums) {
                    if (raw <= 0 || raw > 0xff) continue;
                    int mask = isAnd ? (~raw & 0xff) : raw;
                    if (mask == 0 || mask == 0xff) continue;
                    for (int b = 0; b < 8; b++) {
                        if ((mask & (1 << b)) == 0) continue;
                        String pin = port + b;
                        byPin.computeIfAbsent(pin, k -> new ArrayList<>())
                             .add(kind + (isAnd ? " clear" : " set") + "  "
                                  + f.getName() + "  " + line.trim());
                    }
                }
            }
        }
        di.dispose();

        println("");
        println("### pins the firmware drives, by pin");
        for (Map.Entry<String, List<String>> e : byPin.entrySet()) {
            String pin = e.getKey();
            String note = KNOWN.containsKey(pin) ? KNOWN.get(pin) : "*** UNMAPPED ***";
            println("");
            println("== " + pin + "  (" + note + ")   " + e.getValue().size() + " site(s)");
            LinkedHashSet<String> uniq = new LinkedHashSet<>(e.getValue());
            int n = 0;
            for (String s : uniq) { println("   " + s); if (++n >= 8) break; }
        }
        println("");
        println("### done");
    }
}
