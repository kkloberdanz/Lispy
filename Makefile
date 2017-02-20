DEBUG_FLAGS= -DDEBUG -O0 -Wall -Wextra -g -Wall -Wextra
all:
	gcc lispy.c mpc.c -o lispy -ledit -std=c99 -lm -O2 -Wall -Wextra

debug:
	gcc lispy.c mpc.c -o lispy -ledit -std=c99 -lm $(DEBUG_FLAGS)

clean:
	rm lispy

control:
	gcc control.c mpc.c -o control -ledit -std=c99 -lm -O2 -Wall -Wextra
