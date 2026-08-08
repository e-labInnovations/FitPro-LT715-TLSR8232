// Finds the battery voltage ADC pin and reading function in TLSR8232 firmware.
// Searches for: voltage threshold constants, VBAT/ADC analog registers, percent math.
//@category Analysis

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.*;
import java.util.*;

public class FindBatteryADC extends GhidraScript {

    @Override
    public void run() throws Exception {
        Listing listing = currentProgram.getListing();
        AddressFactory af = currentProgram.getAddressFactory();
        AddressSpace sp = af.getDefaultAddressSpace();
        Memory mem = currentProgram.getMemory();

        println("=== Battery ADC Pin Finder for TLSR8232 ===\n");

        // 1. Search all instructions for battery-voltage-range constants
        // Li-Ion: ~3000mV-4200mV → common constants in code
        // Also ADC raw values: TLSR8232 14-bit ADC with 1.2V ref → battery ~2.7-4.2V
        // Raw = voltage * 4096 / 1200 (approx)
        // 3500mV → ~11944 raw (0x2EA8), 4200mV → ~14336 raw (0x3800)
        // More often firmware uses simpler thresholds like 3600, 3700, 3900, 4100, 4200

        Set<Long> voltageConstants = new HashSet<>(Arrays.asList(
            3500L, 3600L, 3700L, 3800L, 3900L, 4000L, 4100L, 4200L,  // mV values
            0xDACL, 0xE10L, 0xE74L, 0xED8L, 0xF3CL, 0xFA0L, 0x1004L, 0x1068L, // hex mV
            // ADC raw for TLSR8232 with common references:
            0x8CAL, 0x924L, 0x9C4L, 0xA28L,   // raw ADC values range
            // Percent thresholds: 0,5,10,15,20,25,30,40,50,60,70,80,90,100
            5L, 10L, 15L, 20L, 25L, 30L, 40L, 50L, 60L, 70L, 80L, 90L, 100L
        ));

        Map<Long, List<Address>> constRefs = new LinkedHashMap<>();
        InstructionIterator iter = listing.getInstructions(true);
        while (iter.hasNext()) {
            Instruction instr = iter.next();
            for (int i = 0; i < instr.getNumOperands(); i++) {
                Scalar s = instr.getScalar(i);
                if (s == null) continue;
                long v = s.getUnsignedValue();
                if (voltageConstants.contains(v)) {
                    constRefs.computeIfAbsent(v, x -> new ArrayList<>()).add(instr.getAddress());
                }
            }
        }

        println("Voltage/percent threshold constants found in instructions:");
        for (Map.Entry<Long, List<Address>> e : constRefs.entrySet()) {
            if (e.getValue().size() < 10) { // skip very common values like 100
                println(String.format("  %d (0x%x): %s", e.getKey(), e.getKey(), e.getValue()));
            }
        }

        // 2. Search flash for battery-range threshold values stored as 16-bit LE literals
        println("\nSearching flash for 16-bit voltage threshold literals:");
        int[] thresholds = {3500, 3600, 3650, 3700, 3750, 3800, 3850, 3900, 4000, 4100, 4200};
        byte[] flashData = new byte[0x40000];
        mem.getBytes(sp.getAddress(0x20000000L), flashData);
        for (int t : thresholds) {
            byte lo = (byte)(t & 0xff);
            byte hi = (byte)((t >> 8) & 0xff);
            for (int off = 0; off < flashData.length - 1; off++) {
                if (flashData[off] == lo && flashData[off+1] == hi) {
                    println(String.format("  %d mV (0x%04x) as LE16 at flash 0x%08x",
                        t, t, 0x20000000 + off));
                }
            }
        }

        // 3. Look for the PA_IE pattern that shows which PA pin is analog
        // PA_IE value in flash: if a bit is 0 = that pin is in analog (non-digital) mode
        // The key battery pin should have IE=0, OEN=1 (input), GPIO=0 (analog func)
        // Search for characteristic init sequences near PA_IE writes

        // 4. Check what's at the TLSR8232 ADC analog register area
        // For internal VBAT on TLSR8232: no GPIO needed, uses internal path
        // For external GPIO ADC: the pin is configured in the ADC init function
        // The TLSR8232 ADC channel numbers:
        // ana_reg 0x04 bits: pin selection for ADC input
        // Channel 0=PA0, 1=PA3, 2=PB7, 3=PB6, 4=GND, 5=VBAT(internal), etc.

        println("\nLooking for ADC channel configuration bytes (ana_reg ~0x04/0x06):");
        // These would appear as small byte constants (0-15) loaded before analog_write calls
        // Typical battery channel on TLSR8232: channel 5 or 0xB (VBAT internal)
        // or channel 1 (PA3), channel 2 (PB7)

        // 5. Find the function that most likely reads battery
        // It should: be called from multiple places, return a small integer (0-100)
        // Look for functions that reference both ADC registers and return values 0-100
        FunctionManager fm = currentProgram.getFunctionManager();
        println("\nFunctions with ADC/voltage-range constants:");
        for (Map.Entry<Long, List<Address>> e : constRefs.entrySet()) {
            for (Address a : e.getValue()) {
                Function f = fm.getFunctionContaining(a);
                if (f != null) {
                    println(String.format("  const=%d at 0x%08x in function: %s @ %s",
                        e.getKey(), a.getOffset(), f.getName(), f.getEntryPoint()));
                }
            }
        }
    }
}
