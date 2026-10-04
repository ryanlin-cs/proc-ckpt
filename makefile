CC=gcc
CFLAGS=-g3 -O0

build: checkpoint readckpt

checkpoint: ckpt.c libckpt.so
	${CC} ${CFLAGS} -o $@ $<

libckpt.so: libckpt.o
	${CC} ${CFLAGS} -shared -fPIC -o $@ $<

libckpt.o: libckpt.c
	if uname -a | grep -i Darwin; then \
	  echo '*** This is Darwin/Apple, not GNU: Incompatible constructors';\
	else \
	  ${CC} ${CFLAGS} -fPIC -c $<; \
	fi

readckpt: readckpt.c
	${CC} ${CFLAGS} -o $@ $<

check: build
	rm -f myckpt				# Remove any older 'myckpt' files
	./checkpoint ./counting-test 10 &	# Start the target program in the background
	sleep 2
	kill -12 `pgrep -n counting-test`	# Send SIGUSR2 to the pid of 'counting-test'
	sleep 2
	pkill -9 counting-test			# Terminate'counting-test'
	./readckpt myckpt			# Inspect the checkpoint image

clean:
	rm -f myckpt checkpoint libckpt.o libckpt.so readckpt



# check: checkpoint hello 
# 	rm -f myckpt  # Remove any older 'myckpt' files
# 	./checkpoint $(word 2,$^)&  # Start the target program in background
# 	sleep 2
# 	echo Checkpointing ...
# 	kill -12 `pgrep -n $(word 2,$^)`  # Send a SIGUSR2 to the pid
# 	# sleep 2
# 	# echo Restarting ...
# 	# pkill -9 $(word 2,$^) # Kill program
# 	# ./restart myckpt
