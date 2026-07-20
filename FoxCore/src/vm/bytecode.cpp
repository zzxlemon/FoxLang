#include "bytecode.hpp"

// This VM name is FVM

// ============================================================
// Chunk implementation
// ============================================================
void Chunk::write(uint8_t byte, int line) {
    code.push_back(byte);
    lines.push_back(line);
}

void Chunk::writeOp(OpCode op, int line) {
    write(static_cast<uint8_t>(op), line);
}

void Chunk::writeInt(uint32_t val, int line) {
    write(static_cast<uint8_t>(val & 0xFF), line);
    write(static_cast<uint8_t>((val >> 8) & 0xFF), line);
    write(static_cast<uint8_t>((val >> 16) & 0xFF), line);
    write(static_cast<uint8_t>((val >> 24) & 0xFF), line);
}

void Chunk::writeShort(uint16_t val, int line) {
    write(static_cast<uint8_t>(val & 0xFF), line);
    write(static_cast<uint8_t>((val >> 8) & 0xFF), line);
}

void Chunk::writeByte(uint8_t val, int line) {
    write(val, line);
}

uint32_t Chunk::readInt(size_t offset) const {
    if (offset + 4 > code.size()) return 0;
    return static_cast<uint32_t>(code[offset]) |
           (static_cast<uint32_t>(code[offset + 1]) << 8) |
           (static_cast<uint32_t>(code[offset + 2]) << 16) |
           (static_cast<uint32_t>(code[offset + 3]) << 24);
}

uint16_t Chunk::readShort(size_t offset) const {
    if (offset + 2 > code.size()) return 0;
    return static_cast<uint16_t>(code[offset]) |
           (static_cast<uint16_t>(code[offset + 1]) << 8);
}

uint8_t Chunk::readByte(size_t offset) const {
    if (offset >= code.size()) return 0;
    return code[offset];
}

int Chunk::addConstant(const Value& val) {
    constants.push_back(val);
    return static_cast<int>(constants.size() - 1);
}

int Chunk::addConstantString(const std::string& str) {
    return addConstant(Value(str));
}

size_t Chunk::addConstantDouble(double val) {
    constants.push_back(Value(val));
    return constants.size() - 1;
}

size_t Chunk::addConstantInt(int val) {
    constants.push_back(Value(val));
    return constants.size() - 1;
}

void Chunk::patchJump(size_t offset, size_t target) {
    int32_t jumpOffset = static_cast<int32_t>(target - offset - 4);
    code[offset]     = static_cast<uint8_t>(jumpOffset & 0xFF);
    code[offset + 1] = static_cast<uint8_t>((jumpOffset >> 8) & 0xFF);
    code[offset + 2] = static_cast<uint8_t>((jumpOffset >> 16) & 0xFF);
    code[offset + 3] = static_cast<uint8_t>((jumpOffset >> 24) & 0xFF);
}

void Chunk::patchJumpOffset(size_t jumpInstrOffset, int32_t offset) {
    code[jumpInstrOffset]     = static_cast<uint8_t>(offset & 0xFF);
    code[jumpInstrOffset + 1] = static_cast<uint8_t>((offset >> 8) & 0xFF);
    code[jumpInstrOffset + 2] = static_cast<uint8_t>((offset >> 16) & 0xFF);
    code[jumpInstrOffset + 3] = static_cast<uint8_t>((offset >> 24) & 0xFF);
}

std::vector<uint8_t> Chunk::serialize() const {
    std::vector<uint8_t> data;

    auto write32 = [&](uint32_t v) {
        data.push_back(static_cast<uint8_t>(v & 0xFF));
        data.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        data.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
        data.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    };

    // Constants
    write32(static_cast<uint32_t>(constants.size()));
    for (const auto& v : constants) {
        switch (v.getType()) {
        case Value::Type::Int: {
            data.push_back(0);
            int32_t iv = v.asInt();
            data.push_back(static_cast<uint8_t>(iv & 0xFF));
            data.push_back(static_cast<uint8_t>((iv >> 8) & 0xFF));
            data.push_back(static_cast<uint8_t>((iv >> 16) & 0xFF));
            data.push_back(static_cast<uint8_t>((iv >> 24) & 0xFF));
            break;
        }
        case Value::Type::Double: {
            data.push_back(1);
            double dv = v.asDouble();
            uint64_t bits;
            std::memcpy(&bits, &dv, sizeof(bits));
            for (int i = 0; i < 8; i++) {
                data.push_back(static_cast<uint8_t>((bits >> (i * 8)) & 0xFF));
            }
            break;
        }
        case Value::Type::String: {
            data.push_back(2);
            const std::string& sv = v.asString();
            write32(static_cast<uint32_t>(sv.size()));
            for (char c : sv) data.push_back(static_cast<uint8_t>(c));
            break;
        }
        case Value::Type::Array: {
            data.push_back(3);
            const auto& arr = v.asArray();
            write32(static_cast<uint32_t>(arr.size()));
            // Simplified: store array elements as constants recursively
            // For now, empty array placeholder
            break;
        }
        case Value::Type::Void: {
            data.push_back(4);
            break;
        }
        }
    }

    // Code
    write32(static_cast<uint32_t>(code.size()));
    data.insert(data.end(), code.begin(), code.end());

    return data;
}

bool Chunk::deserialize(const std::vector<uint8_t>& data) {
    size_t offset = 0;
    auto read32 = [&]() -> uint32_t {
        if (offset + 4 > data.size()) return 0;
        uint32_t v = static_cast<uint32_t>(data[offset]) |
                     (static_cast<uint32_t>(data[offset + 1]) << 8) |
                     (static_cast<uint32_t>(data[offset + 2]) << 16) |
                     (static_cast<uint32_t>(data[offset + 3]) << 24);
        offset += 4;
        return v;
    };

    // Constants
    uint32_t constCount = read32();
    constants.clear();
    for (uint32_t i = 0; i < constCount; i++) {
        if (offset >= data.size()) return false;
        uint8_t type = data[offset++];
        switch (type) {
        case 0: { // int
            if (offset + 4 > data.size()) return false;
            int32_t iv = static_cast<int32_t>(data[offset]) |
                         (static_cast<int32_t>(data[offset + 1]) << 8) |
                         (static_cast<int32_t>(data[offset + 2]) << 16) |
                         (static_cast<int32_t>(data[offset + 3]) << 24);
            offset += 4;
            constants.push_back(Value(iv));
            break;
        }
        case 1: { // double
            if (offset + 8 > data.size()) return false;
            uint64_t bits = 0;
            for (int j = 0; j < 8; j++) {
                bits |= (static_cast<uint64_t>(data[offset + j]) << (j * 8));
            }
            offset += 8;
            double dv;
            std::memcpy(&dv, &bits, sizeof(dv));
            constants.push_back(Value(dv));
            break;
        }
        case 2: { // string
            uint32_t len = read32();
            if (offset + len > data.size()) return false;
            std::string sv(reinterpret_cast<const char*>(data.data() + offset), len);
            offset += len;
            constants.push_back(Value(sv));
            break;
        }
        case 3: { // array
            uint32_t arrLen = read32();
            std::vector<Value> arr;
            for (uint32_t j = 0; j < arrLen; j++) {
                // nested array elements not fully supported in serialization
                arr.push_back(Value());
            }
            constants.push_back(Value(arr));
            break;
        }
        default: // nil/void
            constants.push_back(Value());
            break;
        }
    }

    // Code
    uint32_t codeSize = read32();
    code.clear();
    if (offset + codeSize > data.size()) return false;
    code.insert(code.end(), data.begin() + offset, data.begin() + offset + codeSize);

    return true;
}

// ============================================================
// CompiledProgram serialization
// ============================================================
std::vector<uint8_t> CompiledProgram::serialize() const {
    std::vector<uint8_t> data;

    auto write32 = [&](uint32_t v) {
        data.push_back(static_cast<uint8_t>(v & 0xFF));
        data.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        data.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
        data.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    };

    // Magic "FOXC"
    data.push_back('F'); data.push_back('O'); data.push_back('X'); data.push_back('C');
    // Version
    write32(1);

    // Import entries
    write32(static_cast<uint32_t>(imports.size()));
    for (const auto& imp : imports) {
        write32(static_cast<uint32_t>(imp.libName.size()));
        for (char c : imp.libName) data.push_back(static_cast<uint8_t>(c));
        write32(static_cast<uint32_t>(imp.alias.size()));
        for (char c : imp.alias) data.push_back(static_cast<uint8_t>(c));
    }

    // Number of functions
    write32(static_cast<uint32_t>(functions.size()));

    for (const auto& func : functions) {
        // Function name
        write32(static_cast<uint32_t>(func.name.size()));
        for (char c : func.name) data.push_back(static_cast<uint8_t>(c));

        // Return type
        write32(static_cast<uint32_t>(func.returnType.size()));
        for (char c : func.returnType) data.push_back(static_cast<uint8_t>(c));

        // Parameters
        write32(static_cast<uint32_t>(func.parameters.size()));
        for (const auto& param : func.parameters) {
            write32(static_cast<uint32_t>(param.name.size()));
            for (char c : param.name) data.push_back(static_cast<uint8_t>(c));
            write32(static_cast<uint32_t>(param.type.size()));
            for (char c : param.type) data.push_back(static_cast<uint8_t>(c));
        }

        // Local count
        write32(static_cast<uint32_t>(func.localCount));

        // Chunk
        std::vector<uint8_t> chunkData = func.chunk.serialize();
        data.insert(data.end(), chunkData.begin(), chunkData.end());
    }

    return data;
}

CompiledProgram CompiledProgram::deserialize(const std::vector<uint8_t>& data) {
    CompiledProgram prog;
    size_t offset = 0;

    auto read32 = [&]() -> uint32_t {
        if (offset + 4 > data.size()) return 0;
        uint32_t v = static_cast<uint32_t>(data[offset]) |
                     (static_cast<uint32_t>(data[offset + 1]) << 8) |
                     (static_cast<uint32_t>(data[offset + 2]) << 16) |
                     (static_cast<uint32_t>(data[offset + 3]) << 24);
        offset += 4;
        return v;
    };

    auto readStr = [&]() -> std::string {
        uint32_t len = read32();
        if (offset + len > data.size()) return "";
        std::string s(reinterpret_cast<const char*>(data.data() + offset), len);
        offset += len;
        return s;
    };

    // Magic check
    if (offset + 4 > data.size()) {
        throw std::runtime_error("Invalid .fc file: too short");
    }
    if (data[offset] != 'F' || data[offset+1] != 'O' || data[offset+2] != 'X' || data[offset+3] != 'C') {
        throw std::runtime_error("Invalid .fc file: bad magic");
    }
    offset += 4;

    // Version
    uint32_t version = read32();
    if (version != 1) {
        throw std::runtime_error("Unsupported .fc version: " + std::to_string(version));
    }

    // Import entries
    uint32_t importCount = read32();
    for (uint32_t i = 0; i < importCount; i++) {
        ImportEntry ie;
        ie.libName = readStr();
        ie.alias = readStr();
        prog.imports.push_back(ie);
    }
    prog.restoreImports();

    // Number of functions
    uint32_t funcCount = read32();

    for (uint32_t i = 0; i < funcCount; i++) {
        CompiledFunction cf;
        cf.name = readStr();
        cf.returnType = readStr();

        uint32_t paramCount = read32();
        for (uint32_t j = 0; j < paramCount; j++) {
            Parameter p;
            p.name = readStr();
            p.type = readStr();
            cf.parameters.push_back(p);
        }

        cf.localCount = static_cast<int>(read32());

        // Deserialize chunk
        // First, we need to know the chunk's serialized size
        // The chunk serialization starts with constant count (4) + constant data + code size (4) + code
        // We'll let the chunk's deserialize handle parsing from current offset
        // But we need to know the size... let's compute it
        // For simplicity, we'll parse the chunk data size by scanning through constants

        size_t chunkStart = offset;

        // Read constants
        if (offset + 4 > data.size()) throw std::runtime_error("Corrupt .fc: no constant count");
        uint32_t constCount = read32();
        for (uint32_t j = 0; j < constCount; j++) {
            if (offset >= data.size()) throw std::runtime_error("Corrupt .fc: constant data");
            uint8_t type = data[offset++];
            switch (type) {
            case 0: offset += 4; break; // int
            case 1: offset += 8; break; // double
            case 2: { // string
                uint32_t slen = read32();
                offset += slen;
                break;
            }
            case 3: { // array
                uint32_t alen = read32();
                offset += alen * 4; // approximate
                break;
            }
            default: break; // nil
            }
        }

        // Read code size
        if (offset + 4 > data.size()) throw std::runtime_error("Corrupt .fc: no code size");
        uint32_t codeSize = read32();
        offset += codeSize;

        // Now parse the chunk from chunkStart
        std::vector<uint8_t> chunkData(data.begin() + chunkStart, data.begin() + offset);
        cf.chunk.deserialize(chunkData);

        prog.functions.push_back(cf);
        prog.functionIndex[cf.name] = i;
    }

    return prog;
}

void CompiledProgram::restoreImports() const {
    auto& libMgr = LibraryManager::getInstance();
    for (const auto& imp : imports) {
        libMgr.markImported(imp.libName, imp.alias);
    }
}

// ============================================================
// BytecodeCompiler implementation
// ============================================================

void BytecodeCompiler::skipWhitespace(Lexer& lexer, Token& token) {
    while (token.type != TOKEN_EOF && !token.value.empty()
        && isspace(static_cast<unsigned char>(token.value[0]))
        && token.value[0] != '\n') {
        token = lexer.nextToken();
    }
}

void BytecodeCompiler::eat(Lexer& lexer, Token& token, TokenT expected) {
    if (token.type == expected) {
        token = lexer.nextToken();
        skipWhitespace(lexer, token);
    } else {
        throw std::runtime_error("BytecodeCompiler: expected token " + std::to_string(expected)
            + ", got " + std::to_string(token.type) + " ('" + token.value + "')");
    }
}

CompiledProgram BytecodeCompiler::compile(const std::string& source, const std::string& filename) {
    program = CompiledProgram();

    // Initialize system libraries before parsing (needed for import resolution)
    RegFunc();

    // Use existing parser to get function definitions
    std::unordered_map<std::string, Value> dummyVars;
    std::unordered_map<std::string, Function> parsedFunctions;

    Parser parser(source, dummyVars, parsedFunctions);
    parser.parseAllFunctions();

    // Record import aliases from LibraryManager (populated by Parser's parseImportStatement)
    auto& libMgr = LibraryManager::getInstance();
    for (const auto& [alias, libName] : libMgr.getAliasMap()) {
        ImportEntry ie;
        ie.libName = libName;
        ie.alias = alias;
        program.imports.push_back(ie);
    }

    // Compile each function
    for (const auto& [funcName, func] : parsedFunctions) {
        CompiledFunction cf;
        cf.name = func.name;
        cf.returnType = func.returnType;
        cf.parameters = func.parameters;
        cf.localCount = static_cast<int>(func.parameters.size()); // params are locals

        // Reserve slots for parameters as locals
        for (size_t i = 0; i < func.parameters.size(); i++) {
            // Opcodes for locals use slot indices 0..N-1
        }

        compileFunctionBody(cf, func.body);

        // Ensure every function ends with a return
        if (cf.chunk.code.empty() || cf.chunk.code.back() != static_cast<uint8_t>(OpCode::OP_RETURN)) {
            cf.chunk.writeOp(OpCode::OP_RETURN, 0);
        }

        program.functions.push_back(cf);
        program.functionIndex[cf.name] = program.functions.size() - 1;
    }

    return program;
}

void BytecodeCompiler::compileFunctionBody(CompiledFunction& cf, const std::vector<std::string>& body) {
    for (const auto& line : body) {
        if (line.empty()) continue;

        int lineNum = cf.chunk.code.size() > 0 ? 0 : 0;

        Lexer lexer(line);
        Token token = lexer.nextToken();
        skipWhitespace(lexer, token);

        if (token.type == TOKEN_EOF) continue;

        switch (token.type) {
        case TOKEN_PRINT: {
            eat(lexer, token, TOKEN_PRINT);
            eat(lexer, token, TOKEN_LPAREN);
            compileExpression(cf, lexer, token);
            eat(lexer, token, TOKEN_RPAREN);
            cf.chunk.writeOp(OpCode::OP_PRINT, lineNum);
            break;
        }
        case TOKEN_IF: {
            compileIfStatement(cf, lexer, token, lineNum);
            break;
        }
        case TOKEN_WHILE: {
            compileWhileStatement(cf, lexer, token, lineNum);
            break;
        }
        case TOKEN_FOR: {
            compileForStatement(cf, lexer, token, lineNum);
            break;
        }
        case TOKEN_RET: {
            eat(lexer, token, TOKEN_RET);
            if (token.type != TOKEN_NEWLINE && token.type != TOKEN_EOF && token.type != TOKEN_RBRACE) {
                compileExpression(cf, lexer, token);
            }
            cf.chunk.writeOp(OpCode::OP_RETURN, lineNum);
            break;
        }
        case TOKEN_END: {
            cf.chunk.writeOp(OpCode::OP_ENDLN, lineNum);
            break;
        }
        case TOKEN_EXIT: {
            eat(lexer, token, TOKEN_EXIT);
            eat(lexer, token, TOKEN_LPAREN);
            compileExpression(cf, lexer, token);
            eat(lexer, token, TOKEN_RPAREN);
            cf.chunk.writeOp(OpCode::OP_EXIT, lineNum);
            break;
        }
        case TOKEN_INPUT: {
            eat(lexer, token, TOKEN_INPUT);
            eat(lexer, token, TOKEN_LPAREN);
            eat(lexer, token, TOKEN_RPAREN);
            eat(lexer, token, TOKEN_ARROW);
            std::string varName = token.value;
            eat(lexer, token, TOKEN_IDENTIFIER);
            cf.chunk.writeOp(OpCode::OP_INPUT, lineNum);
            int nameIdx = cf.chunk.addConstantString(varName);
            cf.chunk.writeInt(static_cast<uint32_t>(nameIdx), lineNum);
            cf.chunk.writeOp(OpCode::OP_DEF_GLOBAL, lineNum);
            cf.chunk.writeInt(static_cast<uint32_t>(nameIdx), lineNum);
            break;
        }
        case TOKEN_IDENTIFIER: {
            std::string identName = token.value;
            eat(lexer, token, TOKEN_IDENTIFIER);

            // Handle uppercase END (same as lowercase end)
            if ((identName == "END" || identName == "end") && token.type != TOKEN_LPAREN && token.type != TOKEN_EQUAL) {
                cf.chunk.writeOp(OpCode::OP_ENDLN, lineNum);
                break;
            }

            if (token.type == TOKEN_LPAREN) {
                // Function call: funcName(...)
                compileCall(cf, lexer, token, identName);
            } else if (token.type == TOKEN_EQUAL) {
                // Assignment: var = expr
                compileAssignment(cf, lexer, token, identName);
            } else if (token.type == TOKEN_LBRACKET) {
                // Array index assignment: arr[index] = expr
                eat(lexer, token, TOKEN_LBRACKET);
                compileExpression(cf, lexer, token);
                eat(lexer, token, TOKEN_RBRACKET);
                // Load array variable
                int nameIdx = cf.chunk.addConstantString(identName);
                cf.chunk.writeOp(OpCode::OP_GET_GLOBAL, lineNum);
                cf.chunk.writeInt(static_cast<uint32_t>(nameIdx), lineNum);
                skipWhitespace(lexer, token);
                eat(lexer, token, TOKEN_EQUAL);
                // Value to set
                compileExpression(cf, lexer, token);
                cf.chunk.writeOp(OpCode::OP_INDEX_SET, lineNum);
            } else {
                throw std::runtime_error("Compile error: unexpected token after identifier '" + identName + "': " + token.value);
            }
            break;
        }
        default:
            throw std::runtime_error("Compile error: unexpected token '" + token.value + "' (" + std::to_string(token.type) + ")");
        }

        // Pop leftover value if any
        if (token.type == TOKEN_NEWLINE || token.type == TOKEN_EOF) {
            // OK
        }
    }

}

void BytecodeCompiler::compileIfStatement(CompiledFunction& cf, Lexer& lexer, Token& token, int lineNum) {
    eat(lexer, token, TOKEN_IF);
    eat(lexer, token, TOKEN_LPAREN);

    // Compile condition
    compileExpression(cf, lexer, token);
    eat(lexer, token, TOKEN_RPAREN);

    // Emit conditional jump (placeholder, will be patched)
    size_t jumpInstr = cf.chunk.code.size();
    cf.chunk.writeOp(OpCode::OP_JMP_IF_FALSE, lineNum);
    cf.chunk.writeInt(0, lineNum); // placeholder offset

    eat(lexer, token, TOKEN_LBRACE);

    // Parse and compile body statements
    std::vector<std::string> body;
    parseBodyStatements(lexer, token, body);
    compileFunctionBody(cf, body);

    // Patch the jump to right after body
    size_t afterBody = cf.chunk.code.size();
    cf.chunk.patchJump(jumpInstr + 1, afterBody);
}

void BytecodeCompiler::compileWhileStatement(CompiledFunction& cf, Lexer& lexer, Token& token, int lineNum) {
    size_t loopStart = cf.chunk.code.size();

    eat(lexer, token, TOKEN_WHILE);
    eat(lexer, token, TOKEN_LPAREN);

    // Compile condition
    compileExpression(cf, lexer, token);
    eat(lexer, token, TOKEN_RPAREN);

    // Emit conditional jump to exit
    size_t exitJump = cf.chunk.code.size();
    cf.chunk.writeOp(OpCode::OP_JMP_IF_FALSE, lineNum);
    cf.chunk.writeInt(0, lineNum); // placeholder

    eat(lexer, token, TOKEN_LBRACE);

    std::vector<std::string> body;
    parseBodyStatements(lexer, token, body);
    compileFunctionBody(cf, body);

    // Loop back
    size_t afterBody = cf.chunk.code.size();
    cf.chunk.writeOp(OpCode::OP_LOOP, lineNum);
    int32_t loopOffset = static_cast<int32_t>(loopStart) - static_cast<int32_t>(afterBody + 5);
    cf.chunk.writeInt(static_cast<uint32_t>(loopOffset), lineNum);

    // Patch exit jump to skip past the OP_LOOP instruction
    cf.chunk.patchJump(exitJump + 1, afterBody + 5);
}

void BytecodeCompiler::compileForStatement(CompiledFunction& cf, Lexer& lexer, Token& token, int lineNum) {
    eat(lexer, token, TOKEN_FOR);
    eat(lexer, token, TOKEN_LPAREN);

    // Parse init statement
    std::string init;
    while (token.type != TOKEN_SEMICOLON && token.type != TOKEN_EOF) {
        init += token.value + " ";
        token = lexer.nextToken();
        skipWhitespace(lexer, token);
    }
    eat(lexer, token, TOKEN_SEMICOLON);

    // Parse condition
    std::string condition;
    while (token.type != TOKEN_SEMICOLON && token.type != TOKEN_EOF) {
        condition += token.value + " ";
        token = lexer.nextToken();
        skipWhitespace(lexer, token);
    }
    eat(lexer, token, TOKEN_SEMICOLON);

    // Parse increment
    std::string iter;
    while (token.type != TOKEN_RPAREN && token.type != TOKEN_EOF) {
        iter += token.value + " ";
        token = lexer.nextToken();
        skipWhitespace(lexer, token);
    }
    eat(lexer, token, TOKEN_RPAREN);


    // Compile init
    if (!init.empty()) {
        Lexer initLexer(init);
        Token initToken = initLexer.nextToken();
        skipWhitespace(initLexer, initToken);
        if (initToken.type == TOKEN_IDENTIFIER) {
            std::string varName = initToken.value;
            initToken = initLexer.nextToken();
            skipWhitespace(initLexer, initToken);
            if (initToken.type == TOKEN_EQUAL) {
                compileAssignment(cf, initLexer, initToken, varName);
            }
        }
    }

    size_t loopStart = cf.chunk.code.size();

    // Compile condition
    if (!condition.empty()) {
        Lexer condLexer(condition);
        Token condToken = condLexer.nextToken();
        skipWhitespace(condLexer, condToken);
        compileExpression(cf, condLexer, condToken);
    } else {
        cf.chunk.writeOp(OpCode::OP_TRUE, lineNum);
    }

    size_t exitJump = cf.chunk.code.size();
    cf.chunk.writeOp(OpCode::OP_JMP_IF_FALSE, lineNum);
    cf.chunk.writeInt(0, lineNum);

    eat(lexer, token, TOKEN_LBRACE);

    std::vector<std::string> body;
    parseBodyStatements(lexer, token, body);
    compileFunctionBody(cf, body);

    // Compile increment
    if (!iter.empty()) {
        Lexer iterLexer(iter);
        Token iterToken = iterLexer.nextToken();
        skipWhitespace(iterLexer, iterToken);
        if (iterToken.type == TOKEN_IDENTIFIER) {
            std::string varName = iterToken.value;
            iterToken = iterLexer.nextToken();
            skipWhitespace(iterLexer, iterToken);
            if (iterToken.type == TOKEN_EQUAL) {
                compileAssignment(cf, iterLexer, iterToken, varName);
            }
        }
    }

    // Loop back
    size_t afterBody = cf.chunk.code.size();
    cf.chunk.writeOp(OpCode::OP_LOOP, lineNum);
    int32_t loopOffset = static_cast<int32_t>(loopStart) - static_cast<int32_t>(afterBody + 5);
    cf.chunk.writeInt(static_cast<uint32_t>(loopOffset), lineNum);

    cf.chunk.patchJump(exitJump + 1, afterBody + 5);
}

std::string BytecodeCompiler::typeStr(Value::Type t) {
    switch (t) {
    case Value::Type::Int: return "int";
    case Value::Type::Double: return "double";
    case Value::Type::String: return "string";
    case Value::Type::Array: return "array";
    default: return "void";
    }
}

void BytecodeCompiler::typeError(const std::string& msg) {
    throw std::runtime_error("TypeError: " + msg);
}

void BytecodeCompiler::compileAssignment(CompiledFunction& cf, Lexer& lexer, Token& token, const std::string& varName) {
    eat(lexer, token, TOKEN_EQUAL);
    Value::Type rhsType = compileExpression(cf, lexer, token);
    varTypes[varName] = rhsType;
    int nameIdx = cf.chunk.addConstantString(varName);
    cf.chunk.writeOp(OpCode::OP_DEF_GLOBAL, 0);
    cf.chunk.writeInt(static_cast<uint32_t>(nameIdx), 0);
}

Value::Type BytecodeCompiler::compileExpression(CompiledFunction& cf, Lexer& lexer, Token& token) {
    Value::Type left = compileCompare(cf, lexer, token);

    while (token.type == TOKEN_AND || token.type == TOKEN_OR) {
        TokenT opType = token.type;
        eat(lexer, token, opType);

        if (opType == TOKEN_AND) {
            size_t jumpInstr = cf.chunk.code.size();
            cf.chunk.writeOp(OpCode::OP_JMP_IF_FALSE, 0);
            cf.chunk.writeInt(0, 0);
            compileCompare(cf, lexer, token);
            size_t endJump = cf.chunk.code.size();
            cf.chunk.writeOp(OpCode::OP_JMP, 0);
            cf.chunk.writeInt(0, 0);
            size_t afterJump = cf.chunk.code.size();
            cf.chunk.patchJump(jumpInstr + 1, afterJump);
            cf.chunk.patchJump(endJump + 1, cf.chunk.code.size());
        } else if (opType == TOKEN_OR) {
            size_t jumpInstr = cf.chunk.code.size();
            cf.chunk.writeOp(OpCode::OP_JMP_IF_FALSE, 0);
            cf.chunk.writeInt(0, 0);
            cf.chunk.writeOp(OpCode::OP_TRUE, 0);
            size_t endJump = cf.chunk.code.size();
            cf.chunk.writeOp(OpCode::OP_JMP, 0);
            cf.chunk.writeInt(0, 0);
            size_t afterJump = cf.chunk.code.size();
            cf.chunk.patchJump(jumpInstr + 1, afterJump);
            compileCompare(cf, lexer, token);
            cf.chunk.patchJump(endJump + 1, cf.chunk.code.size());
        }
        left = Value::Type::Int;
    }
    return left;
}

Value::Type BytecodeCompiler::compileCompare(CompiledFunction& cf, Lexer& lexer, Token& token) {
    Value::Type left = compileAdd(cf, lexer, token);

    while (token.type == TOKEN_EQ || token.type == TOKEN_NE ||
           token.type == TOKEN_GT || token.type == TOKEN_LT ||
           token.type == TOKEN_GE || token.type == TOKEN_LE) {
        TokenT opType = token.type;
        eat(lexer, token, opType);
        Value::Type right = compileAdd(cf, lexer, token);

        if (left != Value::Type::Unknown && right != Value::Type::Unknown &&
            left != Value::Type::Void && right != Value::Type::Void) {
            if ((left == Value::Type::Double && right == Value::Type::Int) ||
                (left == Value::Type::Int && right == Value::Type::Double)) {
                typeError("Cannot compare " + typeStr(left) + " and " + typeStr(right)
                    + " without explicit cast (use int() or double())");
            }
        }

        OpCode opcode;
        switch (opType) {
        case TOKEN_EQ: opcode = OpCode::OP_EQ; break;
        case TOKEN_NE: opcode = OpCode::OP_NE; break;
        case TOKEN_GT: opcode = OpCode::OP_GT; break;
        case TOKEN_LT: opcode = OpCode::OP_LT; break;
        case TOKEN_GE: opcode = OpCode::OP_GE; break;
        case TOKEN_LE: opcode = OpCode::OP_LE; break;
        default: throw std::runtime_error("Unexpected comparison operator");
        }
        cf.chunk.writeOp(opcode, 0);
        left = Value::Type::Int;
    }
    return left;
}

Value::Type BytecodeCompiler::compileAdd(CompiledFunction& cf, Lexer& lexer, Token& token) {
    Value::Type left = compileMul(cf, lexer, token);

    while (token.type == TOKEN_PLUS || token.type == TOKEN_MINUS) {
        TokenT opType = token.type;
        eat(lexer, token, opType);
        Value::Type right = compileMul(cf, lexer, token);

        if (left != Value::Type::Unknown && right != Value::Type::Unknown &&
            left != Value::Type::Void && right != Value::Type::Void) {
            if ((left == Value::Type::Double && right == Value::Type::Int) ||
                (left == Value::Type::Int && right == Value::Type::Double)) {
                typeError("Cannot " + std::string(opType == TOKEN_PLUS ? "add" : "subtract") + " "
                    + typeStr(left) + " and " + typeStr(right)
                    + " without explicit cast (use int() or double())");
            }
            if (left == Value::Type::String && right != Value::Type::String) {
                typeError("Cannot add string and " + typeStr(right));
            }
            if (left != Value::Type::String && right == Value::Type::String) {
                typeError("Cannot add " + typeStr(left) + " and string");
            }
        }

        if (opType == TOKEN_PLUS) {
            cf.chunk.writeOp(OpCode::OP_ADD, 0);
        } else {
            cf.chunk.writeOp(OpCode::OP_SUB, 0);
        }

        if (left == Value::Type::Double || right == Value::Type::Double) {
            left = Value::Type::Double;
        } else if (left == Value::Type::Int && right == Value::Type::Int) {
            left = Value::Type::Int;
        } else if (left == Value::Type::String && right == Value::Type::String) {
            left = Value::Type::String;
        } else {
            left = Value::Type::Unknown;
        }
    }
    return left;
}

Value::Type BytecodeCompiler::compileMul(CompiledFunction& cf, Lexer& lexer, Token& token) {
    return compileUnary(cf, lexer, token);
}

Value::Type BytecodeCompiler::compileUnary(CompiledFunction& cf, Lexer& lexer, Token& token) {
    if (token.type == TOKEN_MINUS) {
        eat(lexer, token, TOKEN_MINUS);
        Value::Type t = compilePrimary(cf, lexer, token);
        cf.chunk.writeOp(OpCode::OP_NEGATE, 0);
        return t;
    } else if (token.type == TOKEN_INT_CAST || token.type == TOKEN_DOUBLE_CAST) {
        CastType castType = (token.type == TOKEN_INT_CAST) ? CastType::Int : CastType::Double;
        compileCastExpr(cf, lexer, token, castType);
        return (castType == CastType::Int) ? Value::Type::Int : Value::Type::Double;
    } else if (token.type == TOKEN_INPUT) {
        eat(lexer, token, TOKEN_INPUT);
        cf.chunk.writeOp(OpCode::OP_INPUT, 0);
        return Value::Type::Unknown;
    } else {
        return compilePrimary(cf, lexer, token);
    }
}

void BytecodeCompiler::compileCastExpr(CompiledFunction& cf, Lexer& lexer, Token& token, CastType castType) {
    TokenT expected = (castType == CastType::Int) ? TOKEN_INT_CAST : TOKEN_DOUBLE_CAST;
    eat(lexer, token, expected);
    eat(lexer, token, TOKEN_LPAREN);
    compileExpression(cf, lexer, token);
    eat(lexer, token, TOKEN_RPAREN);
    cf.chunk.writeOp(
        (castType == CastType::Int) ? OpCode::OP_CAST_INT : OpCode::OP_CAST_DOUBLE,
        0
    );
}

Value::Type BytecodeCompiler::compilePrimary(CompiledFunction& cf, Lexer& lexer, Token& token) {
    skipWhitespace(lexer, token);

    if (token.type == TOKEN_NUMBER) {
        int val = std::stoi(token.value);
        int idx = cf.chunk.addConstant(Value(val));
        cf.chunk.writeOp(OpCode::OP_CONSTANT, 0);
        cf.chunk.writeInt(static_cast<uint32_t>(idx), 0);
        eat(lexer, token, TOKEN_NUMBER);
        return Value::Type::Int;
    } else if (token.type == TOKEN_DOUBLE_NUM) {
        double val = std::stod(token.value);
        int idx = cf.chunk.addConstant(Value(val));
        cf.chunk.writeOp(OpCode::OP_CONSTANT, 0);
        cf.chunk.writeInt(static_cast<uint32_t>(idx), 0);
        eat(lexer, token, TOKEN_DOUBLE_NUM);
        return Value::Type::Double;
    } else if (token.type == TOKEN_STRING) {
        int idx = cf.chunk.addConstantString(token.value);
        cf.chunk.writeOp(OpCode::OP_CONSTANT, 0);
        cf.chunk.writeInt(static_cast<uint32_t>(idx), 0);
        eat(lexer, token, TOKEN_STRING);
        return Value::Type::String;
    } else if (token.type == TOKEN_IDENTIFIER) {
        std::string identName = token.value;
        eat(lexer, token, TOKEN_IDENTIFIER);

        if (token.type == TOKEN_LPAREN) {
            compileCall(cf, lexer, token, identName);
            return Value::Type::Unknown;
        } else if (token.type == TOKEN_LBRACKET) {
            int nameIdx = cf.chunk.addConstantString(identName);
            cf.chunk.writeOp(OpCode::OP_GET_GLOBAL, 0);
            cf.chunk.writeInt(static_cast<uint32_t>(nameIdx), 0);
            eat(lexer, token, TOKEN_LBRACKET);
            compileExpression(cf, lexer, token);
            eat(lexer, token, TOKEN_RBRACKET);
            cf.chunk.writeOp(OpCode::OP_INDEX_GET, 0);
            return Value::Type::Unknown;
        } else {
            int nameIdx = cf.chunk.addConstantString(identName);
            cf.chunk.writeOp(OpCode::OP_GET_GLOBAL, 0);
            cf.chunk.writeInt(static_cast<uint32_t>(nameIdx), 0);
            auto it = varTypes.find(identName);
            if (it != varTypes.end()) return it->second;
            return Value::Type::Unknown;
        }
    } else if (token.type == TOKEN_LBRACKET) {
        eat(lexer, token, TOKEN_LBRACKET);
        int elemCount = 0;
        while (token.type != TOKEN_RBRACKET && token.type != TOKEN_EOF) {
            compileExpression(cf, lexer, token);
            elemCount++;
            if (token.type == TOKEN_COMMA) {
                eat(lexer, token, TOKEN_COMMA);
            }
        }
        eat(lexer, token, TOKEN_RBRACKET);
        cf.chunk.writeOp(OpCode::OP_ARRAY, 0);
        cf.chunk.writeByte(static_cast<uint8_t>(elemCount), 0);
        return Value::Type::Array;
    } else if (token.type == TOKEN_LPAREN) {
        eat(lexer, token, TOKEN_LPAREN);
        Value::Type t = compileExpression(cf, lexer, token);
        eat(lexer, token, TOKEN_RPAREN);
        return t;
    } else {
        throw std::runtime_error("Compile error: unexpected token '" + token.value + "' in expression");
    }
}

void BytecodeCompiler::compileCall(CompiledFunction& cf, Lexer& lexer, Token& token, const std::string& funcName) {
    eat(lexer, token, TOKEN_LPAREN);

    int argCount = 0;
    while (token.type != TOKEN_RPAREN && token.type != TOKEN_EOF) {
        compileExpression(cf, lexer, token);
        argCount++;
        if (token.type == TOKEN_COMMA) {
            eat(lexer, token, TOKEN_COMMA);
            skipWhitespace(lexer, token);
        }
    }
    eat(lexer, token, TOKEN_RPAREN);

    // Push function name as constant
    int nameIdx = cf.chunk.addConstantString(funcName);
    cf.chunk.writeOp(OpCode::OP_CONSTANT, 0);
    cf.chunk.writeInt(static_cast<uint32_t>(nameIdx), 0);

    cf.chunk.writeOp(OpCode::OP_CALL, 0);
    cf.chunk.writeByte(static_cast<uint8_t>(argCount), 0);
}

void BytecodeCompiler::parseBodyStatements(Lexer& lexer, Token& token, std::vector<std::string>& body) {
    while (token.type != TOKEN_RBRACE && token.type != TOKEN_EOF) {
        std::string stmt = parseSingleStmt(lexer, token);
        if (!stmt.empty()) {
            body.push_back(stmt);
        }
        skipWhitespace(lexer, token);
    }
    if (token.type == TOKEN_RBRACE) {
        eat(lexer, token, TOKEN_RBRACE);
    }
}

std::string BytecodeCompiler::parseSingleStmt(Lexer& lexer, Token& token) {
    skipWhitespace(lexer, token);

    if (token.type == TOKEN_EOF || token.type == TOKEN_RBRACE) {
        return "";
    }

    std::string stmt;

    // Handle if/while/for blocks
    if (token.type == TOKEN_IF || token.type == TOKEN_WHILE || token.type == TOKEN_FOR) {
        int braceDepth = 0;
        bool insideBlock = false;
        while (token.type != TOKEN_EOF) {
            if (token.type == TOKEN_STRING) {
                stmt += "\"" + token.value + "\"";
            } else {
                stmt += token.value + " ";
            }
            if (token.type == TOKEN_LBRACE) {
                insideBlock = true;
                braceDepth++;
            } else if (token.type == TOKEN_RBRACE && insideBlock) {
                braceDepth--;
                if (braceDepth == 0) {
                    token = lexer.nextToken();
                    skipWhitespace(lexer, token);
                    break;
                }
            }
            token = lexer.nextToken();
            skipWhitespace(lexer, token);
        }
    } else {
        while (token.type != TOKEN_EOF && token.type != TOKEN_NEWLINE && token.type != TOKEN_RBRACE) {
            if (token.type == TOKEN_STRING) {
                stmt += "\"" + token.value + "\"";
            } else {
                stmt += token.value + " ";
            }
            token = lexer.nextToken();
            skipWhitespace(lexer, token);
        }
        if (token.type == TOKEN_NEWLINE) {
            token = lexer.nextToken();
        }
    }

    size_t start = stmt.find_first_not_of(" \t\n\r");
    size_t end = stmt.find_last_not_of(" \t\n\r");
    if (start != std::string::npos && end != std::string::npos) {
        stmt = stmt.substr(start, end - start + 1);
    } else {
        stmt.clear();
    }
    return stmt;
}

// ============================================================
// VM implementation
// ============================================================

VM::VM() : runtimeError(false) {}

void VM::loadProgram(const CompiledProgram& prog) {
    program = prog;
    globals.clear();
    stack.clear();
    frames.clear();
    runtimeError = false;
}

void VM::resetStack() {
    stack.clear();
}

Value VM::peek(int distance) {
    if (stack.empty()) {
        runtimeErr("Stack empty");
        return Value();
    }
    return stack[stack.size() - 1 - distance];
}

Value VM::pop() {
    if (stack.empty()) {
        runtimeErr("Stack underflow");
        return Value();
    }
    Value val = stack.back();
    stack.pop_back();
    return val;
}

void VM::push(const Value& val) {
    stack.push_back(val);
}

void VM::runtimeErr(const std::string& msg) {
    if (!runtimeError) {
        std::cerr << "[Runtime Error] " << msg << std::endl;
        runtimeError = true;
    }
}

bool VM::callSystemFunction(const std::string& name, int argCount) {
    std::vector<Value> args;
    for (int i = argCount - 1; i >= 0; i--) {
        args.insert(args.begin(), pop());
    }
    // Pop function name
    pop();

    Value result = executeSystemCall(name, args);

    if (!runtimeError) {
        push(result);
    }
    return true;
}

Value VM::executeSystemCall(const std::string& funcName, const std::vector<Value>& args) {
    Interpreter sys;
    if (sys.isSystemFunction(funcName)) {
        return sys.SystemFunctionBuildIn(funcName, args);
    }
    runtimeErr("Unknown system function: " + funcName);
    return Value();
}

bool VM::callFunction(const std::string& name, int argCount) {
    auto it = program.functionIndex.find(name);
    if (it == program.functionIndex.end()) {
        // Check if it's a system function
        Interpreter sys;
        if (sys.isSystemFunction(name)) {
            return callSystemFunction(name, argCount);
        }
        runtimeErr("Undefined function: " + name);
        return false;
    }

    const CompiledFunction& func = program.functions[it->second];

    // Check arg count
    if (argCount != static_cast<int>(func.parameters.size())) {
        runtimeErr("Function " + name + " expects " + std::to_string(func.parameters.size())
            + " arguments, got " + std::to_string(argCount));
        return false;
    }

    // Pop function name
    pop();

    // Create new frame
    CallFrame frame;
    frame.function = &func;
    frame.ip = 0;

    // Store arguments as locals
    frame.locals.resize(func.localCount);
    for (int i = argCount - 1; i >= 0; i--) {
        frame.locals[i] = pop();
    }

    frames.push_back(frame);
    return true;
}

void VM::run() {
    if (program.functions.empty()) {
        runtimeErr("No functions to execute");
        return;
    }

    // Find main function
    auto it = program.functionIndex.find("main");
    if (it == program.functionIndex.end()) {
        runtimeErr("No 'main' function found");
        return;
    }

    resetStack();
    frames.clear();
    runtimeError = false;

    // Initialize system libraries
    RegFunc();

    // Start with main function
    CallFrame frame;
    frame.function = &program.functions[it->second];
    frame.ip = 0;
    frame.locals.resize(frame.function->localCount);
    frames.push_back(frame);

    // Execution loop
    while (!frames.empty() && !runtimeError) {
        CallFrame& cf = frames.back();
        const Chunk& chunk = cf.function->chunk;

        if (cf.ip >= chunk.code.size()) {
            frames.pop_back();
            if (!frames.empty()) {
                push(Value()); // void return value
            }
            continue;
        }

        uint8_t instruction = chunk.code[cf.ip++];

        switch (static_cast<OpCode>(instruction)) {
        case OpCode::OP_RETURN: {
            Value retVal = Value();
            if (!stack.empty()) {
                retVal = pop();
            }
            frames.pop_back();
            if (!frames.empty()) {
                push(retVal);
            }
            break;
        }
        case OpCode::OP_CONSTANT: {
            uint32_t idx = chunk.readInt(cf.ip);
            cf.ip += 4;
            if (idx < chunk.constants.size()) {
                push(chunk.constants[idx]);
            } else {
                runtimeErr("Constant index out of bounds");
            }
            break;
        }
        case OpCode::OP_NEGATE: {
            Value v = pop();
            if (v.getType() == Value::Type::Int) {
                push(Value(-v.asInt()));
            } else if (v.getType() == Value::Type::Double) {
                push(Value(-v.asDouble()));
            } else {
                runtimeErr("Cannot negate non-numeric value");
            }
            break;
        }
        case OpCode::OP_ADD: {
            Value b = pop();
            Value a = pop();
            if (a.getType() == Value::Type::Int && b.getType() == Value::Type::Int) {
                push(Value(a.asInt() + b.asInt()));
            } else if (a.getType() == Value::Type::Double && b.getType() == Value::Type::Double) {
                push(Value(a.asDouble() + b.asDouble()));
            } else if (a.getType() == Value::Type::String && b.getType() == Value::Type::String) {
                push(Value(a.asString() + b.asString()));
            } else if (a.getType() == Value::Type::Int && b.getType() == Value::Type::Double) {
                push(Value(static_cast<double>(a.asInt()) + b.asDouble()));
            } else if (a.getType() == Value::Type::Double && b.getType() == Value::Type::Int) {
                push(Value(a.asDouble() + static_cast<double>(b.asInt())));
            } else {
                runtimeErr("Type mismatch in addition");
            }
            break;
        }
        case OpCode::OP_SUB: {
            Value b = pop();
            Value a = pop();
            if (a.getType() == Value::Type::Int && b.getType() == Value::Type::Int) {
                push(Value(a.asInt() - b.asInt()));
            } else if (a.getType() == Value::Type::Double && b.getType() == Value::Type::Double) {
                push(Value(a.asDouble() - b.asDouble()));
            } else if (a.getType() == Value::Type::Int && b.getType() == Value::Type::Double) {
                push(Value(static_cast<double>(a.asInt()) - b.asDouble()));
            } else if (a.getType() == Value::Type::Double && b.getType() == Value::Type::Int) {
                push(Value(a.asDouble() - static_cast<double>(b.asInt())));
            } else {
                runtimeErr("Type mismatch in subtraction");
            }
            break;
        }
        case OpCode::OP_MUL: {
            Value b = pop();
            Value a = pop();
            if (a.getType() == Value::Type::Int && b.getType() == Value::Type::Int) {
                push(Value(a.asInt() * b.asInt()));
            } else if (a.getType() == Value::Type::Double && b.getType() == Value::Type::Double) {
                push(Value(a.asDouble() * b.asDouble()));
            } else if (a.getType() == Value::Type::Int && b.getType() == Value::Type::Double) {
                push(Value(static_cast<double>(a.asInt()) * b.asDouble()));
            } else if (a.getType() == Value::Type::Double && b.getType() == Value::Type::Int) {
                push(Value(a.asDouble() * static_cast<double>(b.asInt())));
            } else {
                runtimeErr("Type mismatch in multiplication");
            }
            break;
        }
        case OpCode::OP_DIV: {
            Value b = pop();
            Value a = pop();
            if (a.getType() == Value::Type::Int && b.getType() == Value::Type::Int) {
                if (b.asInt() == 0) { runtimeErr("Division by zero"); break; }
                push(Value(a.asInt() / b.asInt()));
            } else if (a.getType() == Value::Type::Double && b.getType() == Value::Type::Double) {
                if (b.asDouble() == 0.0) { runtimeErr("Division by zero"); break; }
                push(Value(a.asDouble() / b.asDouble()));
            } else if (a.getType() == Value::Type::Int && b.getType() == Value::Type::Double) {
                if (b.asDouble() == 0.0) { runtimeErr("Division by zero"); break; }
                push(Value(static_cast<double>(a.asInt()) / b.asDouble()));
            } else if (a.getType() == Value::Type::Double && b.getType() == Value::Type::Int) {
                if (b.asInt() == 0) { runtimeErr("Division by zero"); break; }
                push(Value(a.asDouble() / static_cast<double>(b.asInt())));
            } else {
                runtimeErr("Type mismatch in division");
            }
            break;
        }

        case OpCode::OP_TRUE: push(Value(1)); break;
        case OpCode::OP_FALSE: push(Value(0)); break;
        case OpCode::OP_NIL: push(Value()); break;
        case OpCode::OP_NOT: {
            Value v = pop();
            push(Value(v.asBool() ? 0 : 1));
            break;
        }
        case OpCode::OP_EQ: {
            Value b = pop();
            Value a = pop();
            bool result = false;
            if (a.getType() == Value::Type::Int && b.getType() == Value::Type::Int) {
                result = (a.asInt() == b.asInt());
            } else if (a.getType() == Value::Type::Double && b.getType() == Value::Type::Double) {
                result = (a.asDouble() == b.asDouble());
            } else if (a.getType() == Value::Type::String && b.getType() == Value::Type::String) {
                result = (a.asString() == b.asString());
            } else if ((a.getType() == Value::Type::Int || a.getType() == Value::Type::Double) &&
                       (b.getType() == Value::Type::Int || b.getType() == Value::Type::Double)) {
                double al = (a.getType() == Value::Type::Int) ? static_cast<double>(a.asInt()) : a.asDouble();
                double bl = (b.getType() == Value::Type::Int) ? static_cast<double>(b.asInt()) : b.asDouble();
                result = (al == bl);
            } else {
                runtimeErr("Type mismatch in equality comparison");
            }
            push(Value(result ? 1 : 0));
            break;
        }
        case OpCode::OP_NE: {
            Value b = pop();
            Value a = pop();
            bool result = false;
            if (a.getType() == Value::Type::Int && b.getType() == Value::Type::Int) {
                result = (a.asInt() != b.asInt());
            } else if (a.getType() == Value::Type::Double && b.getType() == Value::Type::Double) {
                result = (a.asDouble() != b.asDouble());
            } else if (a.getType() == Value::Type::String && b.getType() == Value::Type::String) {
                result = (a.asString() != b.asString());
            } else if ((a.getType() == Value::Type::Int || a.getType() == Value::Type::Double) &&
                       (b.getType() == Value::Type::Int || b.getType() == Value::Type::Double)) {
                double al = (a.getType() == Value::Type::Int) ? static_cast<double>(a.asInt()) : a.asDouble();
                double bl = (b.getType() == Value::Type::Int) ? static_cast<double>(b.asInt()) : b.asDouble();
                result = (al != bl);
            } else {
                runtimeErr("Type mismatch in inequality comparison");
            }
            push(Value(result ? 1 : 0));
            break;
        }
        case OpCode::OP_GT:
        case OpCode::OP_LT:
        case OpCode::OP_GE:
        case OpCode::OP_LE: {
            Value b = pop();
            Value a = pop();
            double al = (a.getType() == Value::Type::Int) ? static_cast<double>(a.asInt()) : a.asDouble();
            double bl = (b.getType() == Value::Type::Int) ? static_cast<double>(b.asInt()) : b.asDouble();
            bool result = false;
            switch (static_cast<OpCode>(instruction)) {
            case OpCode::OP_GT: result = (al > bl); break;
            case OpCode::OP_LT: result = (al < bl); break;
            case OpCode::OP_GE: result = (al >= bl); break;
            case OpCode::OP_LE: result = (al <= bl); break;
            default: break;
            }
            push(Value(result ? 1 : 0));
            break;
        }
        case OpCode::OP_PRINT: {
            Value v = pop();
            switch (v.getType()) {
            case Value::Type::Int: std::cout << v.asInt(); break;
            case Value::Type::Double: std::cout << v.asDouble(); break;
            case Value::Type::String: std::cout << v.asString(); break;
            case Value::Type::Void: break;
            case Value::Type::Array: std::cout << "[array]"; break;
            }
            break;
        }
        case OpCode::OP_PRINTLN: {
            Value v = pop();
            switch (v.getType()) {
            case Value::Type::Int: std::cout << v.asInt(); break;
            case Value::Type::Double: std::cout << v.asDouble(); break;
            case Value::Type::String: std::cout << v.asString(); break;
            case Value::Type::Void: break;
            case Value::Type::Array: std::cout << "[array]"; break;
            }
            std::cout << std::endl;
            break;
        }
        case OpCode::OP_ENDLN: {
            std::cout << std::endl;
            break;
        }
        case OpCode::OP_POP: {
            pop();
            break;
        }
        case OpCode::OP_DEF_GLOBAL: {
            uint32_t nameIdx = chunk.readInt(cf.ip);
            cf.ip += 4;
            if (nameIdx >= chunk.constants.size() ||
                chunk.constants[nameIdx].getType() != Value::Type::String) {
                runtimeErr("Invalid global variable name constant");
                break;
            }
            std::string name = chunk.constants[nameIdx].asString();
            Value val = pop();
            globals[name] = val;
            break;
        }
        case OpCode::OP_GET_GLOBAL: {
            uint32_t nameIdx = chunk.readInt(cf.ip);
            cf.ip += 4;
            if (nameIdx >= chunk.constants.size() ||
                chunk.constants[nameIdx].getType() != Value::Type::String) {
                runtimeErr("Invalid global variable name constant");
                break;
            }
            std::string name = chunk.constants[nameIdx].asString();
            auto it = globals.find(name);
            if (it == globals.end()) {
                runtimeErr("Undefined variable: " + name);
                break;
            }
            push(it->second);
            break;
        }
        case OpCode::OP_SET_GLOBAL: {
            uint32_t nameIdx = chunk.readInt(cf.ip);
            cf.ip += 4;
            if (nameIdx >= chunk.constants.size() ||
                chunk.constants[nameIdx].getType() != Value::Type::String) {
                runtimeErr("Invalid global variable name constant");
                break;
            }
            std::string name = chunk.constants[nameIdx].asString();
            Value val = peek();
            auto it = globals.find(name);
            if (it == globals.end()) {
                runtimeErr("Undefined variable: " + name);
                break;
            }
            it->second = val;
            break;
        }
        case OpCode::OP_GET_LOCAL: {
            uint8_t slot = chunk.readByte(cf.ip);
            cf.ip++;
            if (slot >= cf.locals.size()) {
                runtimeErr("Local variable slot out of bounds");
                break;
            }
            push(cf.locals[slot]);
            break;
        }
        case OpCode::OP_SET_LOCAL: {
            uint8_t slot = chunk.readByte(cf.ip);
            cf.ip++;
            if (slot >= cf.locals.size()) {
                runtimeErr("Local variable slot out of bounds");
                break;
            }
            cf.locals[slot] = peek();
            break;
        }
        case OpCode::OP_JMP: {
            int32_t offset = static_cast<int32_t>(chunk.readInt(cf.ip));
            cf.ip += 4;
            cf.ip += offset;
            break;
        }
        case OpCode::OP_JMP_IF_FALSE: {
            int32_t offset = static_cast<int32_t>(chunk.readInt(cf.ip));
            cf.ip += 4;
            Value cond = pop();
            if (!cond.asBool()) {
                cf.ip += offset;
            }
            break;
        }
        case OpCode::OP_LOOP: {
            int32_t offset = static_cast<int32_t>(chunk.readInt(cf.ip));
            cf.ip += 4;
            cf.ip += offset;
            break;
        }
        case OpCode::OP_CALL: {
            uint8_t argCount = chunk.readByte(cf.ip);
            cf.ip++;

            if (static_cast<int>(stack.size()) < argCount + 1) {
                runtimeErr("Stack underflow in function call");
                break;
            }

            // Pop function name (on top of stack, above args)
            Value fnVal = pop();
            if (fnVal.getType() != Value::Type::String) {
                runtimeErr("Function name must be a string");
                break;
            }
            std::string fnName = fnVal.asString();

            auto progIt = program.functionIndex.find(fnName);
            if (progIt != program.functionIndex.end()) {
                const CompiledFunction& func = program.functions[progIt->second];
                if (argCount != static_cast<uint8_t>(func.parameters.size())) {
                    runtimeErr("Function " + fnName + " expects " + std::to_string(func.parameters.size())
                        + " arguments, got " + std::to_string(argCount));
                    break;
                }

                // Pop args (pushed left-to-right, arg0 is bottommost)
                std::vector<Value> args(argCount);
                for (int i = argCount - 1; i >= 0; i--) {
                    args[i] = pop();
                }

                // Set parameters as global variables (matching existing interpreter behavior)
                for (size_t i = 0; i < func.parameters.size(); i++) {
                    globals[func.parameters[i].name] = args[i];
                }

                // Create new frame to execute function
                frames.push_back(CallFrame{});
                CallFrame& newFrame = frames.back();
                newFrame.function = &func;
                newFrame.ip = 0;
                newFrame.locals.resize(func.localCount);
            } else {
                Interpreter sys;
                if (sys.isSystemFunction(fnName)) {
                    std::vector<Value> args(argCount);
                    for (int i = argCount - 1; i >= 0; i--) {
                        args[i] = pop();
                    }
                    Value result = sys.SystemFunctionBuildIn(fnName, args);
                    push(result);
                } else {
                    runtimeErr("Undefined function: " + fnName);
                }
            }
            break;
        }
        case OpCode::OP_INPUT: {
            std::string userInput;
            std::getline(std::cin, userInput);
            push(Value(userInput));
            break;
        }
        case OpCode::OP_CAST_INT: {
            Value v = pop();
            switch (v.getType()) {
            case Value::Type::Int: push(Value(v.asInt())); break;
            case Value::Type::Double: push(Value(static_cast<int>(v.asDouble()))); break;
            case Value::Type::String: {
                try { push(Value(std::stoi(v.asString()))); }
                catch (...) { runtimeErr("Cannot cast string to int"); }
                break;
            }
            default: runtimeErr("Cannot cast to int"); break;
            }
            break;
        }
        case OpCode::OP_CAST_DOUBLE: {
            Value v = pop();
            switch (v.getType()) {
            case Value::Type::Int: push(Value(static_cast<double>(v.asInt()))); break;
            case Value::Type::Double: push(Value(v.asDouble())); break;
            case Value::Type::String: {
                try { push(Value(std::stod(v.asString()))); }
                catch (...) { runtimeErr("Cannot cast string to double"); }
                break;
            }
            default: runtimeErr("Cannot cast to double"); break;
            }
            break;
        }
        case OpCode::OP_ARRAY: {
            uint8_t elemCount = chunk.readByte(cf.ip);
            cf.ip++;
            std::vector<Value> elements;
            for (int i = elemCount - 1; i >= 0; i--) {
                elements.push_back(stack[stack.size() - 1 - i]);
            }
            for (int i = 0; i < elemCount; i++) pop();
            push(Value(elements));
            break;
        }
        case OpCode::OP_INDEX_GET: {
            Value index = pop();
            Value arr = pop();
            if (arr.getType() != Value::Type::Array) {
                runtimeErr("Index target is not an array");
                break;
            }
            if (index.getType() != Value::Type::Int) {
                runtimeErr("Array index must be an integer");
                break;
            }
            int idx = index.asInt();
            const std::vector<Value>& elements = arr.asArray();
            if (idx < 0 || idx >= static_cast<int>(elements.size())) {
                runtimeErr("Array index out of bounds");
                break;
            }
            push(elements[idx]);
            break;
        }
        case OpCode::OP_INDEX_SET: {
            Value value = pop();
            Value index = pop();
            Value arr = pop();
            if (arr.getType() != Value::Type::Array) {
                runtimeErr("Index target is not an array");
                break;
            }
            if (index.getType() != Value::Type::Int) {
                runtimeErr("Array index must be an integer");
                break;
            }
            int idx = index.asInt();
            std::vector<Value>& elements = const_cast<std::vector<Value>&>(arr.asArray());
            if (idx < 0 || idx >= static_cast<int>(elements.size())) {
                runtimeErr("Array index out of bounds");
                break;
            }
            elements[idx] = value;
            push(value);
            break;
        }
        case OpCode::OP_EXIT: {
            Value code = pop();
            if (code.getType() == Value::Type::Int) {
                std::exit(code.asInt());
            }
            std::exit(0);
            break;
        }
        case OpCode::OP_AND: {
            // Short-circuit AND
            Value left = pop();
            if (!left.asBool()) {
                push(Value(0));
            } else {
                Value right = pop();
                push(Value(right.asBool() ? 1 : 0));
            }
            break;
        }
        case OpCode::OP_OR: {
            Value left = pop();
            if (left.asBool()) {
                push(Value(1));
            } else {
                Value right = pop();
                push(Value(right.asBool() ? 1 : 0));
            }
            break;
        }
        case OpCode::OP_IMPORT: {
            // Import is handled by the compiler stage, skip at runtime
            break;
        }
        case OpCode::OP_HALT: {
            frames.clear();
            break;
        }
        default:
            runtimeErr("Unknown opcode: " + std::to_string(instruction));
            break;
        }
    }
}
