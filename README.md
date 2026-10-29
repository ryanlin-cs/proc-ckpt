# Transparent Process Checkpoint

This project was completed as homework for CS 5600 (Computer Systems) at
Northeastern University, Fall 2026, under Professor Cooperman.

The course provided example code for:

- parsing `/proc/self/maps`
- allocating memory for the stack during restart
- parts of the `makefile`

## Build

Build `checkpoint`, `readckpt`, and `restart` with the default target:

```console
make
```

See `makefile` for all available targets, which you can run with:

```console
make <target>
```

## Usage

To checkpoint a program, pass it and its arguments to `checkpoint`:

```console
./checkpoint <program> [args...]
```

For example, using `test.py`:

```console
./checkpoint python3 test.py
```

Then, from another terminal, send `SIGUSR2` to the process to write its
checkpoint image to `myckpt`:

```console
kill -SIGUSR2 <PID>   # replace <PID> with the process ID
```

To print the memory segments stored in the image:

```console
./readckpt myckpt
```

To restore the process from the image:

```console
./restart myckpt
```

## Limitations

- Works well on CentOS 7 (Linux kernel 3.10), but needs to be extended to work
  on recent Linux distros
- Only works for single-threaded processes
- Does not support checkpoint-restart-checkpoint, i.e., checkpointing a process
  after it has been restarted
