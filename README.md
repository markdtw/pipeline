# pipeline
C implementation of reduced MIPS R3000 ISA machine.

## Project Objective:
- Implement a pipelined, functional processor simulator for the reduced MIPS R3000 ISA.
- Design test case to validate implementation, particularly on hazards handling.


## Compile:
```bash
cd simulator/
make
```

### Input file:
- iimage.bin:<br>
  The instruction image (big-endian, encoded in binary). The first four bytes indicate the initial value of PC. The next four bytes specify the number of words to be loaded into instruction memory. The remaining are the program text to be loaded into I-memory.

- dimage.bin:<br>
  The data image (big-endian format, encoded in binary). The first four bytes show the initial value of $sp. The next four bytes specify the number of words to be loaded into data memory. The remaining are the data to be loaded into D-memory, starting from address 0.

### Output file:
- snapshot.rpt:<br>
  Contains all register values at each cycle.

- error_dump.rpt:<br>
  Contains error messages.
