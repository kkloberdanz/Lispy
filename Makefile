all:
	gcc lispy.c mpc.c -o lispy -ledit -std=c99 -lm -O2

debug:
	gcc lispy.c mpc.c -o lispy -ledit -std=c99 -lm -DDEBUG -O0

clean:
	rm lispy
