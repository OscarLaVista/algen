# IBM Developer's Workframe/2 Make File Creation run at 14:08:18 on 09/03/93

# Make File Creation run in directory:
#   C:\ALGEN;

.SUFFIXES:

.SUFFIXES: .c .cpp .cxx

ALGEN.EXE:  \
  algen.OBJ \
  ALGEN.MAK
   ICC.EXE @<<
 /B" /base:0x10000 /packd /align:4"
 /Fe"ALGEN.EXE" 
algen.OBJ
<<

{.}.c.obj:
   ICC.EXE /Tdc /Sp1 /O /C   .\$*.c

{.}.cpp.obj:
   ICC.EXE /Tdc /Sp1 /O /C   .\$*.cpp

{.}.cxx.obj:
   ICC.EXE /Tdc /Sp1 /O /C   .\$*.cxx

!include ALGEN.DEP
