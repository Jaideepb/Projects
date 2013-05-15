all:
	gcc -gdwarf-2 -g3 -g -o MIPSsim projectMain.c
clean:
	rm -rf MIPSsim generated_disassembly.txt generated_simulation.txt
