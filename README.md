# Cinje

**Cinje** is a modern C injection template for Pokémon FireRed ROMs. It's designed to minimize development friction and improve reliability, so developers can focus on writing code without having to deal with nonsense bugs.

## Key Features

- **Use one thing, and use it well:** entire projects can be developed by writing only C code.
- **Rebuild what has to be rebuilt:** even the most obscure dependencies are tracked, ensuring no regeneration is needed when a target is modified.
- **Builds as fast as development:** blazingly fast incremental builds powered by Ninja.
- **Give me Unix and I am good to go:** Linux, MSYS2 and WSL2 are supported.

## Building

First, generate the build files for Ninja by running `python3 generate.py`. It's only necessary to regenerate when you add or remove files.

Now, run `ninja` to build the code and inject it in a ROM. `BPRE.gba` is used as the base ROM image, it must be present before building. When the build process finishes, the resulting ROM will be written to `BPRE_out.gba`.
