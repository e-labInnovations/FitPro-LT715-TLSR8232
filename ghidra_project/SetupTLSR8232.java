// Ghidra post-import script: sets up TLSR8232 memory regions and triggers disassembly.
//@category Analysis

import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.program.model.address.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;

public class SetupTLSR8232 extends GhidraScript {

    @Override
    public void run() throws Exception {
        Memory mem = currentProgram.getMemory();
        AddressFactory af = currentProgram.getAddressFactory();
        AddressSpace defaultSpace = af.getDefaultAddressSpace();

        // SRAM: 0x00840000, 8KB
        Address sramStart = defaultSpace.getAddress(0x00840000L);
        if (mem.getBlock(sramStart) == null) {
            mem.createUninitializedBlock("SRAM", sramStart, 0x2000, false);
            println("Added SRAM block at 0x00840000");
        }

        // Peripheral registers: 0x00800000, 32KB (volatile)
        Address regStart = defaultSpace.getAddress(0x00800000L);
        if (mem.getBlock(regStart) == null) {
            MemoryBlock regBlock = mem.createUninitializedBlock("REGISTERS", regStart, 0x8000, false);
            regBlock.setVolatile(true);
            println("Added REGISTERS block at 0x00800000");
        }

        // -- Annotate the KNLT header as data, not code --
        Address flashBase = defaultSpace.getAddress(0x20000000L);
        createLabel(flashBase, "flash_header", true);

        // Actual startup code begins after the 32-byte KNLT header
        // Bytes 0x20000000-0x2000001f = header fields (KNLT magic, CRC, copy table)
        // Bytes 0x20000020+ = startup / crt0 code
        Address codeStart = defaultSpace.getAddress(0x20000020L);
        createLabel(codeStart, "startup_code", true);

        // Mark entry as an external entry point so auto-analysis propagates from here
        currentProgram.getSymbolTable().addExternalEntryPoint(codeStart);

        // Force disassembly from startup code
        println("Disassembling from 0x20000020...");
        DisassembleCommand disCmd = new DisassembleCommand(codeStart, null, true);
        disCmd.applyTo(currentProgram, monitor);
        println("Disassembly triggered.");

        // Create a function at the startup entry
        FunctionManager fm = currentProgram.getFunctionManager();
        if (fm.getFunctionAt(codeStart) == null) {
            fm.createFunction("startup", codeStart,
                new AddressSet(codeStart), SourceType.ANALYSIS);
            println("Created startup function at 0x20000020");
        }

        println("Setup complete.");
    }
}
