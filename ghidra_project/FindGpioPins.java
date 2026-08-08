// Finds all writes to TLSR8232 GPIO config registers and decodes pin functions.
//@category Analysis

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.lang.Register;
import java.util.*;

public class FindGpioPins extends GhidraScript {

    // TLSR8232 GPIO register addresses in CPU address space
    static final Map<Long, String> GPIO_REGS = new LinkedHashMap<>();
    static {
        long[] bases = {0x00800580L, 0x00800588L, 0x00800590L, 0x00800598L, 0x008005a0L};
        String[] ports = {"PA", "PB", "PC", "PD", "PE"};
        String[] regs  = {"_IN","_IE","_OEN","_OUT","_POL","_DS","_GPIO","_IRQ"};
        for (int p = 0; p < ports.length; p++)
            for (int r = 0; r < regs.length; r++)
                GPIO_REGS.put(bases[p] + r, ports[p] + regs[r]);
    }

    static String[] PIN_NAMES = {"0","1","2","3","4","5","6","7"};

    @Override
    public void run() throws Exception {
        AddressFactory af = currentProgram.getAddressFactory();
        AddressSpace addrSpace = af.getDefaultAddressSpace();
        Memory mem = currentProgram.getMemory();
        Listing listing = currentProgram.getListing();

        // Collect what values are written to each GPIO register
        // by scanning the flash for 32-bit literal values matching GPIO addresses,
        // then looking at the instruction before the load that uses it.
        // Simpler: scan all instructions for stores whose target is in GPIO range.

        Map<String, Set<Integer>> writtenValues = new LinkedHashMap<>();
        for (String name : GPIO_REGS.values()) writtenValues.put(name, new TreeSet<>());

        // Scan for GPIO address literals in the binary, then look at nearby stores
        byte[] flashData = new byte[(int)Math.min(currentProgram.getMaxAddress().getOffset() -
            0x20000000L + 1, 0x40000)];
        Address flashBase = addrSpace.getAddress(0x20000000L);
        mem.getBytes(flashBase, flashData);

        int found = 0;
        for (Map.Entry<Long, String> entry : GPIO_REGS.entrySet()) {
            long regAddr = entry.getKey();
            String regName = entry.getValue();
            byte[] needle = new byte[]{
                (byte)(regAddr & 0xff),
                (byte)((regAddr >> 8) & 0xff),
                (byte)((regAddr >> 16) & 0xff),
                (byte)((regAddr >> 24) & 0xff)
            };

            int off = 0;
            while (off < flashData.length - 4) {
                int idx = indexOf(flashData, needle, off);
                if (idx == -1) break;
                off = idx + 1;

                // The literal is at flashBase+idx.
                // Look backwards up to 32 bytes for a register write pattern.
                // In TC32, a store byte looks like: tstrb rX, [rY, #0]
                // The value being stored is set up before the store.
                // Scan 2-8 instructions before for a tmov/immediate load.
                long litAddr = 0x20000000L + idx;
                Address litAddrObj = addrSpace.getAddress(litAddr);

                // Find the instruction that references this literal (within 64 bytes before)
                for (int lookback = 2; lookback <= 64; lookback += 2) {
                    Address candAddr = addrSpace.getAddress(litAddr - lookback);
                    Instruction instr = listing.getInstructionAt(candAddr);
                    if (instr == null) continue;
                    // Check if this instruction loads an immediate into a register
                    String mnemonic = instr.getMnemonicString().toLowerCase();
                    if (mnemonic.contains("mov") || mnemonic.contains("strb") ||
                        mnemonic.contains("str") || mnemonic.contains("ldr")) {
                        // Try to get scalar operand value
                        for (int oi = 0; oi < instr.getNumOperands(); oi++) {
                            Scalar s = instr.getScalar(oi);
                            if (s != null && s.getValue() >= 0 && s.getValue() <= 0xff) {
                                writtenValues.get(regName).add((int)s.getValue());
                            }
                        }
                    }
                }
                found++;
            }
        }

        println("=== TLSR8232 GPIO Pin Analysis ===\n");
        println("Known from SDK headers:");
        println("  PC3 = SPI MOSI (display)   PC5 = SPI CLK (display)");
        println("  PC1 = DC (display)          PA6 = RST (display)");
        println("  PA1 = CS (display)          PB3 = Backlight\n");

        println("GPIO register write values found in firmware:");
        String[] ports = {"PA","PB","PC","PD","PE"};
        for (String port : ports) {
            boolean any = false;
            for (String suffix : new String[]{"_OEN","_IE","_GPIO","_OUT","_POL","_DS"}) {
                String regName = port + suffix;
                Set<Integer> vals = writtenValues.get(regName);
                if (vals != null && !vals.isEmpty()) {
                    if (!any) { println(port + ":"); any = true; }
                    for (int v : vals) {
                        StringBuilder pins = new StringBuilder();
                        for (int bit = 0; bit < 8; bit++)
                            if ((v & (1 << bit)) != 0) pins.append(port).append(bit).append(" ");
                        println(String.format("  %-10s = 0x%02x  bits: %s", suffix, v,
                            pins.length() > 0 ? pins.toString().trim() : "(none)"));
                    }
                }
            }
        }

        println("\nTotal GPIO literal refs scanned: " + found);
    }

    private int indexOf(byte[] data, byte[] needle, int fromIndex) {
        outer:
        for (int i = fromIndex; i <= data.length - needle.length; i++) {
            for (int j = 0; j < needle.length; j++)
                if (data[i+j] != needle[j]) continue outer;
            return i;
        }
        return -1;
    }
}
