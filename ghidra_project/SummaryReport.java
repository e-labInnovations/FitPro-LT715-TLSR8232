// Prints a quick summary: function count, defined code units, string count
//@category Analysis

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;

public class SummaryReport extends GhidraScript {
    @Override
    public void run() throws Exception {
        FunctionManager fm = currentProgram.getFunctionManager();
        int funcCount = fm.getFunctionCount();
        println("Functions found: " + funcCount);

        SymbolTable st = currentProgram.getSymbolTable();
        int symCount = st.getNumSymbols();
        println("Symbols: " + symCount);

        Listing listing = currentProgram.getListing();
        long instrCount = listing.getNumInstructions();
        println("Instructions disassembled: " + instrCount);

        long dataCount = listing.getNumDefinedData();
        println("Defined data items: " + dataCount);
    }
}
