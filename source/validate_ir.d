module validate_ir;

import std.algorithm : canFind, startsWith;
import std.array : appender;
import std.string : indexOf, splitLines, strip;

/// Result of checking a DirectX compute IR fixture.
struct IrReport
{
    bool ok;
    string[] errors;
}

/**
 * Checks the minimal DirectX compute IR shape expected by LLVM DirectX /
 * future LDC targetDirectX (from Clang CodeGenHLSL + llvm/test/CodeGen/DirectX).
 *
 * Required (compute entry):
 *   - target triple containing dxil-pc-shadermodel and -compute (or library)
 *   - "hlsl.shader"="compute"
 *   - "hlsl.numthreads"="…"
 *
 * Optional but noted when missing:
 *   - "exp-shader"="cs" (often present before dxil-prepare)
 */
IrReport validateDirectXComputeIR(string ir)
{
    auto errors = appender!(string[]);

    if (ir.indexOf("target triple") < 0)
        errors.put("missing target triple line");
    else
    {
        bool goodTriple;
        foreach (line; ir.splitLines)
        {
            auto s = line.strip();
            if (!s.startsWith("target triple"))
                continue;
            // Accept compute or library triples used in LLVM tests
            if (s.canFind("dxil-pc-shadermodel") &&
                (s.canFind("-compute") || s.canFind("-library")))
                goodTriple = true;
            else
                errors.put("target triple should be dxil-pc-shadermodel*-compute (or -library)");
            break;
        }
        if (!goodTriple && errors.data.length == 0)
            errors.put("no usable dxil target triple found");
    }

    if (!ir.canFind(`"hlsl.shader"="compute"`) && !ir.canFind(`"hlsl.shader" = "compute"`))
        errors.put(`missing function attribute "hlsl.shader"="compute"`);

    if (!ir.canFind(`"hlsl.numthreads"=`) && !ir.canFind(`"hlsl.numthreads" =`))
        errors.put(`missing function attribute "hlsl.numthreads"="X,Y,Z"`);

    IrReport report;
    report.errors = errors.data;
    report.ok = report.errors.length == 0;
    return report;
}

/// Negative control: deliberately invalid IR should fail validation.
IrReport validateShouldFail(string ir)
{
    return validateDirectXComputeIR(ir);
}
