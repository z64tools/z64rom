#define Asm_SymbolAlias(name, symbol) \
    asm (".global " name "\n"         \
    name " = " #symbol)
