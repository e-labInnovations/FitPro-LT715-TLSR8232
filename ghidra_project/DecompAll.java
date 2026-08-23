// Decompiles every defined function in the image to one C file.
//
// This is the "firmware -> C" pass. The output is real, compilable-looking C for
// each function body, but it is not a buildable project: no symbol names, no
// types, no struct layouts, and every hardware register appears as a store
// through a DAT_ pointer. Treat it as a searchable, readable reference for the
// machine code, not as source.
//
// Output: <program name>.c next to the project, plus an index of functions
// ordered by address with their size, so the big ones are easy to find.
//@category Analysis
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.util.task.ConsoleTaskMonitor;
import java.io.*;
import java.util.*;

public class DecompAll extends GhidraScript {

    public void run() throws Exception {
        String out = System.getProperty("decomp.out", "/tmp/firmware_decomp.c");
        DecompInterface di = new DecompInterface();
        DecompileOptions opts = new DecompileOptions();
        di.setOptions(opts);
        di.openProgram(currentProgram);

        List<Function> funcs = new ArrayList<>();
        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) funcs.add(f);
        funcs.sort(Comparator.comparing(Function::getEntryPoint));
        println("### functions: " + funcs.size());

        PrintWriter w = new PrintWriter(new BufferedWriter(new FileWriter(out)));
        w.println("/* Decompiled from " + currentProgram.getName() + " by Ghidra (Telink_TC32).");
        w.println(" * " + funcs.size() + " functions. Machine-generated: no symbol names or types.");
        w.println(" */");
        w.println();
        w.println("/* ---- function index (address  size  name) ---- */");
        for (Function f : funcs)
            w.println("/* " + f.getEntryPoint() + "  " + f.getBody().getNumAddresses() + "  " + f.getName() + " */");
        w.println();

        int ok = 0, fail = 0;
        for (Function f : funcs) {
            DecompileResults res = di.decompileFunction(f, 60, new ConsoleTaskMonitor());
            w.println("/* ======== " + f.getName() + " @ " + f.getEntryPoint()
                      + "  size=" + f.getBody().getNumAddresses() + " ======== */");
            if (res != null && res.decompileCompleted()) {
                w.println(res.getDecompiledFunction().getC());
                ok++;
            } else {
                w.println("/* decompilation failed: "
                          + (res == null ? "null" : res.getErrorMessage()) + " */");
                fail++;
            }
            w.println();
            if ((ok + fail) % 100 == 0) println("   " + (ok + fail) + "/" + funcs.size());
        }
        w.close();
        di.dispose();
        println("### wrote " + out + "  ok=" + ok + " failed=" + fail);
    }
}
