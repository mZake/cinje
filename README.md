## Building

First, activate the development environment by running `source devenv.sh`. While a development environment is active, you can run `deactivate` to exit from it. It's necessary to do this only once per shell session.

Now, generate the build files for Ninja by running `python3 generate.py`. It's only necessary to regenerate when you add or remove files.

Finally, run `ninja` to build the code and inject it in a ROM. `BPRE.gba` is used as the base ROM image, it must be present before building. When the build process finishes, the resulting ROM will be written to `BPRE_out.gba`.
