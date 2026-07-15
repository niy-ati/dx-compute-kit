module app;

import validate_ir;

import std.file : exists, getcwd, isDir, readText, dirEntries, SpanMode;
import std.getopt : config, defaultGetoptPrinter, getopt;
import std.path : absolutePath, buildNormalizedPath, extension;
import std.process : Config, execute, environment;
import std.stdio : stderr, stdout, writeln, writefln;
import std.string : startsWith;

int main(string[] args)
{
    string fixturesDir = buildNormalizedPath(getcwd(), "fixtures");
    string kernelsDir = buildNormalizedPath(getcwd(), "kernels");
    string dxcPath = findDxc();
    bool compileKernel;
    bool runE2e;
    bool quiet;

    auto helpInfo = getopt(args,
        config.passThrough,
        "fixtures", "Directory of .ll fixtures (default: ./fixtures)", &fixturesDir,
        "kernels", "Directory of .hlsl kernels (default: ./kernels)", &kernelsDir,
        "dxc", "Path to dxc.exe", &dxcPath,
        "compile-kernel", "Compile kernels/saxpy.hlsl with dxc to kernels/out/", &compileKernel,
        "e2e", "Compile kernel (if needed) and run host/d3d12_saxpy.exe", &runE2e,
        "quiet|q", "Less output", &quiet,
    );

    if (helpInfo.helpWanted)
    {
        defaultGetoptPrinter(
            "dx-compute-kit — DirectX compute for dcompute/LDC (IR + DXIL + D3D12 saxpy E2E)\n\n"
                ~ "Usage:\n"
                ~ "  dx-compute-kit\n"
                ~ "  dx-compute-kit --compile-kernel\n"
                ~ "  dx-compute-kit --e2e\n"
                ~ "  run_e2e.bat\n\n"
                ~ "Options:",
            helpInfo.options);
        return 0;
    }

    int status;

    if (runE2e)
        compileKernel = true;

    if (compileKernel)
    {
        status = compileSaxpy(kernelsDir, dxcPath, quiet);
        if (status != 0)
            return status;
    }

    if (runE2e)
    {
        status = runD3D12Host(quiet);
        if (status != 0)
            return status;
        if (!quiet)
            writeln("[dx-compute-kit] E2E PASS");
        // still validate fixtures below
    }

    fixturesDir = absolutePath(fixturesDir);
    if (!exists(fixturesDir) || !isDir(fixturesDir))
    {
        stderr.writefln("error: fixtures directory missing: %s", fixturesDir);
        return 1;
    }

    int failed;
    int checked;
    int expectedFails;
    foreach (e; dirEntries(fixturesDir, SpanMode.shallow))
    {
        if (!e.isFile || extension(e.name) != ".ll")
            continue;
        checked++;
        const text = readText(e.name);
        auto report = validateDirectXComputeIR(text);
        import std.path : baseName;
        const expectFail = baseName(e.name).startsWith("bad_");

        if (expectFail)
        {
            expectedFails++;
            if (report.ok)
            {
                failed++;
                writefln("FAIL %s (expected validation errors, got ok)", e.name);
            }
            else if (!quiet)
                writefln("ok   %s (expected fail: %s issue(s))", e.name, report.errors.length);
            continue;
        }

        if (report.ok)
        {
            if (!quiet)
                writefln("ok   %s", e.name);
        }
        else
        {
            failed++;
            writefln("FAIL %s", e.name);
            foreach (msg; report.errors)
                writefln("  - %s", msg);
        }
    }

    if (checked == 0)
    {
        stderr.writeln("error: no .ll fixtures found");
        return 1;
    }

    if (!quiet)
        writefln("checked %s fixture(s) (%s negative), %s unexpected failure(s)",
            checked, expectedFails, failed);
    return failed == 0 ? 0 : 1;
}

int runD3D12Host(bool quiet)
{
    auto host = buildNormalizedPath(getcwd(), "host", "d3d12_saxpy.exe");
    auto dxil = buildNormalizedPath(getcwd(), "kernels", "out", "saxpy.dxil");
    if (!exists(host))
    {
        stderr.writeln("error: host/d3d12_saxpy.exe missing — run host\\build.bat first");
        return 1;
    }
    if (!exists(dxil))
    {
        stderr.writeln("error: kernels/out/saxpy.dxil missing — use --compile-kernel");
        return 1;
    }
    if (!quiet)
        writefln("[dx-compute-kit] running %s", host);
    auto r = execute([host, dxil], null, Config.none);
    if (r.output.length)
        stdout.write(r.output);
    return r.status;
}

string findDxc()
{
    auto fromEnv = environment.get("DXC", null);
    if (fromEnv.length && exists(fromEnv))
        return fromEnv;

    // Common winget install
    auto winget = buildNormalizedPath(
        environment.get("LOCALAPPDATA", ""),
        "Microsoft", "WinGet", "Packages",
        "Microsoft.DirectX.ShaderCompiler_Microsoft.Winget.Source_8wekyb3d8bbwe",
        "bin", "x64", "dxc.exe");
    if (exists(winget))
        return winget;

    return "dxc";
}

int compileSaxpy(string kernelsDir, string dxcPath, bool quiet)
{
    auto hlsl = buildNormalizedPath(kernelsDir, "saxpy.hlsl");
    if (!exists(hlsl))
    {
        stderr.writefln("error: missing %s", hlsl);
        return 1;
    }

    auto outDir = buildNormalizedPath(kernelsDir, "out");
    import std.file : mkdirRecurse;
    mkdirRecurse(outDir);

    auto dxil = buildNormalizedPath(outDir, "saxpy.dxil");
    auto asmOut = buildNormalizedPath(outDir, "saxpy.ll");

    // DXIL object (for future D3D12 PSO)
    auto r1 = execute(
        [dxcPath, "-T", "cs_6_0", "-E", "main", "-Fo", dxil, hlsl],
        null, Config.none);
    if (r1.status != 0)
    {
        stderr.writefln("dxc (DXIL) failed:\n%s", r1.output);
        return r1.status;
    }

    // Human-readable LLVM IR / DXIL asm for metadata inspection
    auto r2 = execute(
        [dxcPath, "-T", "cs_6_0", "-E", "main", "-Fc", asmOut, hlsl],
        null, Config.none);
    if (r2.status != 0)
    {
        // Some dxc builds use -Fc differently; still OK if DXIL succeeded
        if (!quiet)
            stderr.writefln("warn: dxc asm dump failed (DXIL still written):\n%s", r2.output);
    }

    if (!quiet)
    {
        writefln("wrote %s", dxil);
        if (exists(asmOut))
            writefln("wrote %s", asmOut);
    }
    return 0;
}
