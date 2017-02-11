all:
	gcc lispy.c mpc.c -o lispy -ledit -std=c99 -lm

parsing:
	gcc -std=c99 -Wall parsing.c mpc.c -ledit -lm -o parsing
clean:
	rm lispy
