# zecart

Efficient &amp; customizable Tracing framework on top of Pin.

# Whitelists

Currently zecart works solely on whitelisting, it's not possible to blacklist
at the moment. However, whitelisting is fairly effective.

For example, when whitelisting the main module and two shared object names,
only instructions belonging to the code ranges of these modules will make it
to the logs. This could saves a lot of redundant logs of instructions
belonging to the operating system, 3rd party libraries, etc.

# Features

* Modules
    * --main-module           Whitelist the main module.
    * --module <name>         Whitelist modules which match this string.
* Addresses
    * --range <start> <end>   Whitelist this range of memory for code.
    * --inside <start> <end>  Whitelist instructions within a sequence.
* Instructions
    * --ins <mnemonic>        Whitelist an instruction.
* Work in Progress
    * --dump <expr> <len>     Dump a value or buffer, depending on the
                              expression and length.

# Sequences

Inside sequences do not represent a range of memory. Instead, if one wants to
analyze a piece of code that jumps into unknown memory, then this is one of
the methods to analyze it. Take for example the following assembly snippet.

```asm
0x401000: pushad
0x401001: call eax
0x401003: popad
```

In such scenario, it'd make sense to start an inside sequence at 0x401000, and
to stop it at 0x401003.

# Expressions

Expressions are small calculations which represent an address or value based
on the result of the expression. Expressions allow us to dump memory,
registers, etc.
