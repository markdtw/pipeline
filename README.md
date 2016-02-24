# pipeline


* Project Objective:
 - Implement a pipelined, functional processor simulator for the reduced MIPS R3000 ISA.
 - Design test case to validate implementation, particularly on hazards handling.

* Compile:
 - by make file

* Input Format:
 - iimage.bin:
 
 The instruction image (big-endian, encoded in binary). The first 
 four bytes indicate the initial value of PC. The next four bytes specify the
 number of words to be loaded into instruction memory.
 The remaining are the program text to be loaded into I-memory.
 
 - dimage.bin:
 
 The data image (big-endian format, encoded in binary). The first four
 bytes show the initial value of $sp. The next four bytes specify the number of words
 to be loaded into data memory. The remaining are the data to be loaded into D-memory,
 starting from address 0.
 
 - located at testcase folder, don't need to move.

* Output Format:
 - snapshot.rpt:
 
 contains all register values at each cycle.
 
 - error_dump.rpt:
 
 contains error messages.
 
 - located at the root folder.


good luck.
