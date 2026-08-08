// Traces all stores to TLSR8232 GPIO registers and decodes the pin configuration.
//@category Analysis

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.scalar.Scalar;
import java.util.*;

public class GpioPinMap extends GhidraScript {

    static final String[] PORT_NAMES = {"PA","PB","PC","PD","PE"};
    static final long[]   PORT_BASES = {0x800580L, 0x800588L, 0x800590L, 0x800598L, 0x8005a0L};
    static final String[] REG_NAMES  = {"IN","IE","OEN","OUT","POL","DS","GPIO","IRQ"};

    // Bit-to-name helpers based on known SDK pin names
    static String pinName(String port, int bit) {
        // From display.h: PA1=CS, PA6=RST, PB3=BL, PC1=DC, PC3=MOSI, PC5=SCK
        Map<String,String> known = new HashMap<>();
        known.put("PA1","CS(display)");  known.put("PA6","RST(display)");
        known.put("PB3","BL(backlight)");
        known.put("PC1","DC(display)");  known.put("PC3","MOSI(display)"); known.put("PC5","SCK(display)");
        known.put("PA0","SWS/debug");
        String k = port + bit;
        return known.containsKey(k) ? k+"="+known.get(k) : k;
    }

    @Override
    public void run() throws Exception {
        AddressFactory af = currentProgram.getAddressFactory();
        // Build map: register address -> {port, reg suffix}
        Map<Long, String[]> regMap = new LinkedHashMap<>();
        for (int p = 0; p < PORT_NAMES.length; p++)
            for (int r = 0; r < REG_NAMES.length; r++)
                regMap.put(PORT_BASES[p] + r, new String[]{PORT_NAMES[p], REG_NAMES[r]});

        // Scan all instructions: look for strb/str whose destination resolves to a GPIO address
        Listing listing = currentProgram.getListing();
        InstructionIterator iter = listing.getInstructions(true);

        // Track: for each (port, reg) -> list of (addr_written, value_stored)
        // We look for the pattern: load imm into reg, then strb that reg to GPIO addr
        Map<String, List<long[]>> regWrites = new LinkedHashMap<>();
        for (int p = 0; p < PORT_NAMES.length; p++)
            for (int r = 0; r < REG_NAMES.length; r++)
                regWrites.put(PORT_NAMES[p]+"_"+REG_NAMES[r], new ArrayList<>());

        // Scan all references to each GPIO register address
        ReferenceManager refMgr = currentProgram.getReferenceManager();
        for (Map.Entry<Long, String[]> e : regMap.entrySet()) {
            long gpioAddr = e.getKey();
            String port = e.getValue()[0], reg = e.getValue()[1];
            String key = port + "_" + reg;

            // Find references to this address in the default address space
            // (The GPIO registers are in a different space, but referenced as immediates/pointers)
            // Instead, scan instructions near where the address appears as a literal

            // Walk backward from instruction that references the GPIO addr
            // to find the mov/ldr that loads the value
            Address gpioAddrObj = af.getDefaultAddressSpace().getAddress(
                0x20000000L + (gpioAddr - 0x800000L)); // mapped into flash space? No.

            // Actually scan all instructions for stores with known operand values
        }

        // Better approach: scan all strb/str instructions; for each,
        // walk backward to find the value being stored
        Map<String, TreeSet<Integer>> pinConfig = new LinkedHashMap<>();

        iter = listing.getInstructions(true);
        Instruction prev2 = null, prev1 = null;
        while (iter.hasNext()) {
            Instruction instr = iter.next();
            String mnem = instr.getMnemonicString().toLowerCase();

            // Look for store instructions (tstrb, tstr, tst + b suffix)
            if (mnem.startsWith("tstr") || mnem.equals("tst")) {
                // Try to get the memory reference target
                for (Reference ref : instr.getReferencesFrom()) {
                    if (ref.getReferenceType().isData()) {
                        long targetOff = ref.getToAddress().getOffset();
                        if (regMap.containsKey(targetOff)) {
                            String[] info = regMap.get(targetOff);
                            String k = info[0] + "_" + info[1];
                            // Find what value is being stored: look at prev instructions
                            Integer storedVal = findStoredValue(prev2, prev1, instr);
                            if (storedVal != null) {
                                pinConfig.computeIfAbsent(k, x -> new TreeSet<>()).add(storedVal);
                            }
                        }
                    }
                }
            }
            prev2 = prev1;
            prev1 = instr;
        }

        // Output the final pin map
        println("=== TLSR8232 GPIO Pin Configuration from Firmware ===\n");
        println("Known pin assignments (from SDK headers):");
        println("  PA0 = SWS debug interface");
        println("  PA1 = Display CS (chip-select)");
        println("  PA6 = Display RST (reset)");
        println("  PB3 = Display backlight");
        println("  PC1 = Display DC (data/command)");
        println("  PC3 = Display SPI MOSI");
        println("  PC5 = Display SPI SCK");
        println();

        for (int p = 0; p < PORT_NAMES.length; p++) {
            String port = PORT_NAMES[p];
            // Print OEN first (direction), then GPIO mode, then IE (digital input)
            for (String reg : new String[]{"GPIO","OEN","IE","OUT","POL"}) {
                String k = port + "_" + reg;
                TreeSet<Integer> vals = pinConfig.get(k);
                if (vals != null && !vals.isEmpty()) {
                    println(port + "_" + reg + ":");
                    for (int v : vals) {
                        StringBuilder bits = new StringBuilder();
                        for (int bit = 7; bit >= 0; bit--)
                            if ((v & (1<<bit)) != 0)
                                bits.append("  ").append(pinName(port, bit));
                        String bitStr = bits.length() > 0 ? bits.toString().trim() : "(none active)";
                        println(String.format("  wrote 0x%02x [%s%s%s%s%s%s%s%s] -> %s",
                            v,
                            (v&0x80)!=0?"1":"0",(v&0x40)!=0?"1":"0",(v&0x20)!=0?"1":"0",
                            (v&0x10)!=0?"1":"0",(v&0x08)!=0?"1":"0",(v&0x04)!=0?"1":"0",
                            (v&0x02)!=0?"1":"0",(v&0x01)!=0?"1":"0", bitStr));
                    }
                }
            }
        }
        println("\n(OEN=0 → output, OEN=1 → input; GPIO=1 → GPIO mode; IE=1 → input buffer on)");
        println("(OUT = initial output value written; POL = interrupt polarity)");
    }

    Integer findStoredValue(Instruction i2, Instruction i1, Instruction store) {
        // Look at i2 and i1 for a mov/add with an immediate 8-bit value
        for (Instruction i : new Instruction[]{i1, i2}) {
            if (i == null) continue;
            String m = i.getMnemonicString().toLowerCase();
            if (m.contains("mov") || m.contains("add") || m.contains("ldr")) {
                for (int oi = 0; oi < i.getNumOperands(); oi++) {
                    Scalar s = i.getScalar(oi);
                    if (s != null) {
                        long v = s.getUnsignedValue();
                        if (v >= 0 && v <= 0xff) return (int) v;
                    }
                }
            }
        }
        return null;
    }
}
