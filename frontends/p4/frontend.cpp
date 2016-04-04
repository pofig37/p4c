#include <iostream>
#include <fstream>

#include "ir/ir.h"
#include "../common/options.h"
#include "lib/nullstream.h"
#include "lib/path.h"

#include "../common/typeMap.h"
#include "../common/resolveReferences/resolveReferences.h"
// Passes
#include "toP4/toP4.h"
#include "validateParsedProgram.h"
#include "createBuiltins.h"
#include "../common/constantFolding.h"
#include "unusedDeclarations.h"
#include "typeChecking/typeChecker.h"
#include "evaluator/evaluator.h"
#include "evaluator/specializeBlocks.h"
#include "strengthReduction.h"
#include "simplify.h"

// The pv10 flag is necessary because the converter 1.2 -> 1.0 synthesizes some program
// fragments which have no source position (i.e., the position is invalid, or line 0).
// For these the reference resolution that tries to find declarations before uses
// fails.
const IR::P4Program*
run_frontend(const CompilerOptions &options, const IR::P4Program* v12_program, bool p4v10) {
    if (v12_program == nullptr)
        return nullptr;

    Util::PathName path(options.prettyPrintFile);
    std::ostream *ppStream = openFile(path.toString(), true);
    std::ostream *midStream = options.dumpStream("-fe");
    std::ostream *endStream = options.dumpStream("-last");

    P4::ReferenceMap  refMap;  // This is reused many times, since every analysis clear it
    P4::TypeMap       typeMap1, typeMap2, typeMap3;
    P4::BlockSpecList specBlocks;

    PassManager frontend = {
        new P4::ToP4(ppStream, options.file),
        // Simple checks on parsed program
        new P4::ValidateParsedProgram(),
        // Synthesize some built-in constructs
        new P4::CreateBuiltins(),
        // First pass of constant folding, before types are known
        // May be needed to compute types
        new P4::ResolveReferences(&refMap, p4v10, true),
        new P4::ConstantFolding(&refMap, nullptr),
        new P4::ResolveReferences(&refMap, p4v10),
        // Type checking and type inference.  Also inserts
        // explicit casts where implicit casts exist.
        new P4::TypeChecker(&refMap, &typeMap1),
        // Another round of constant folding, using type information.
        new P4::SimplifyControlFlow(),
        new P4::ResolveReferences(&refMap, p4v10),
        new P4::ConstantFolding(&refMap, &typeMap1),
        new P4::StrengthReduction(),
        // TODO: add more

        new PassRepeated{
            // specialization
            new P4::ResolveReferences(&refMap, p4v10),
            new P4::TypeChecker(&refMap, &typeMap1, true, true),
            new P4::FindBlocksToSpecialize(&refMap, &typeMap1, &specBlocks),
            new P4::SpecializeBlocks(&refMap, &specBlocks),

            // constant folding
            new P4::ResolveReferences(&refMap, p4v10),
            new P4::TypeChecker(&refMap, &typeMap2, true, true),
            new P4::ConstantFolding(&refMap, &typeMap2),
        },

        // Print program in the middle
        new P4::ToP4(midStream, options.file),
        new PassRepeated{
            // Remove unused declarations.
            new P4::ResolveReferences(&refMap, p4v10),
            new P4::RemoveUnusedDeclarations(&refMap),
        },
        // Print the program before the end.
        new P4::ToP4(endStream, options.file),
    };

    frontend.setStopOnError(true);
    const IR::P4Program* result = v12_program->apply(frontend);
    return result;
}