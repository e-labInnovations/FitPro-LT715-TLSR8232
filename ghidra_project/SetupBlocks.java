// Prepares a raw TLSR8232 flash image for analysis.
//
// Run as a -preScript on import, before auto-analysis: the memory map has to be
// right first or every literal-pool pointer resolves to nothing.
//
// TC32 fetches code from flash mapped at 0, so the image belongs at base 0 (an
// earlier database used 0x20000000, which made every 0x0000xxxx pointer word in
// the image look like a stray constant). On top of that, two regions the image
// does not contain but the code constantly touches:
//   REGISTERS 0x800000  - peripheral space, incl. GPIO at 0x800580
//   SRAM      0x840000  - 8 KB data
// Both uninitialized, so references resolve and nothing is decoded as code.
//
// The image holds two KNLT applications: the main app at 0 and a second at
// 0x20000. Each starts with a jump, so both are marked as entry points.
//@category Analysis
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.mem.*;

public class SetupBlocks extends GhidraScript {

    public void run() throws Exception {
        Memory mem = currentProgram.getMemory();

        if (mem.getBlock("REGISTERS") == null) {
            MemoryBlock b = mem.createUninitializedBlock("REGISTERS", toAddr(0x800000), 0x8000, false);
            b.setRead(true); b.setWrite(true); b.setVolatile(true);
            println("created REGISTERS 0x800000 +0x8000");
        }
        if (mem.getBlock("SRAM") == null) {
            MemoryBlock b = mem.createUninitializedBlock("SRAM", toAddr(0x840000), 0x2000, false);
            b.setRead(true); b.setWrite(true);
            println("created SRAM 0x840000 +0x2000");
        }

        // KNLT magic marks each application header; entry follows the jump at +0
        long[] apps = {0x0, 0x20000};
        for (long a : apps) {
            Address addr = toAddr(a);
            byte[] magic = new byte[4];
            mem.getBytes(addr.add(8), magic);
            String m = new String(magic);
            println("app @ " + addr + " magic=" + m);
            if (!"KNLT".equals(m)) continue;
            addEntryPoint(addr);
            disassemble(addr);
            createFunction(addr, "app_entry_" + Long.toHexString(a));
        }
        println("### setup done");
    }
}
