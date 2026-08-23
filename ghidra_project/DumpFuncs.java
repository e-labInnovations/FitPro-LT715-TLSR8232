//@category Analysis
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.util.task.ConsoleTaskMonitor;
import java.util.*;

public class DumpFuncs extends GhidraScript {
    public void run() throws Exception {
        String[] targets = getScriptArgs();
        if (targets.length == 0)
            targets = new String[]{"0000cdc4","0000c37c","000066e8","0000216c","0000cf88","0000d1bc"};
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        AddressSpace sp = currentProgram.getAddressFactory().getDefaultAddressSpace();
        for (String t : targets) {
            Address a = sp.getAddress(Long.parseLong(t, 16));
            Function f = getFunctionAt(a);
            if (f == null) { println("### no function at " + t); continue; }
            println("");
            println("################ " + f.getName() + " @ " + a);
            Set<String> callers = new LinkedHashSet<>();
            for (Reference r : getReferencesTo(a)) {
                Function c = getFunctionContaining(r.getFromAddress());
                if (c != null) callers.add(c.getName() + "@" + c.getEntryPoint());
            }
            println("### callers: " + callers);
            println("### calls out to: " + f.getCalledFunctions(new ConsoleTaskMonitor()));
            DecompileResults res = di.decompileFunction(f, 60, new ConsoleTaskMonitor());
            if (res != null && res.decompileCompleted())
                println(res.getDecompiledFunction().getC());
        }
        di.dispose();
        println("### done");
    }
}
