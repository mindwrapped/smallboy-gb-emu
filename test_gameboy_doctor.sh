# Test cpu instructions with gameboy doctor
rm test.log

make

./gb "02-interrupts.gb" > test.log

./gameboy-doctor/gameboy-doctor ./test.log cpu_instrs 2
