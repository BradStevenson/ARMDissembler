ARM Dissembler.
==============

What?
=====

An ARM dissembler I wrote which started as a learning exercise and proceeded to its current level. More functionality is planned. It will dissemble an ELF(Executable Linking Format) file back to its mnemonic.

Usage: 
======

Feel free to test it with any of the sample ELF files provided. The .s files are also provided for error checking/interest etc. I recommend opening these files with an emulator such as KMD (http://brej.org/kmd/) and compare with out put in the 'disassembly' window.

$ DisARM {ELF FILE}

Output:
=======

Information on the current file will be printed followed by the ARM instructions in the format - 

Address in Memory | Hex value | ASCII Value | Disassembly mnemonic

Current Issues:
===============

Currently not handling ELF files containing multiple program headers correctly.

![](https://ts4o2g.dm1.livefilestore.com/y1pXGreMwIPWwWhplw1JTHCYlwxOCCxd5rm4DbrWHMOHYKHsHfPaxqnu0vOEOv8s88VnYeTBvq6gDxylk9wxtLZpjP4ybLCYylW/Ubuntu%2012.png?psid=1)
