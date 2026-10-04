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

check: build counting-test
	rm -f myckpt					# Remove any older 'myckpt' files
	./checkpoint ./counting-test 10 &		# Start the target program in the background
	sleep 2
	cat /proc/`pgrep -n counting-test`/maps		# Print memory segment of target process
	kill -12 `pgrep -n counting-test`		# Send SIGUSR2
	sleep 2
	pkill -9 counting-test				# Terminate target process 
	./readckpt myckpt				# Inspect the checkpoint image

clean:
	rm -f myckpt checkpoint libckpt.o libckpt.so readckpt counting-test

dist: clean
	dir=`basename $$PWD` && cd .. && tar czvf $$dir.tar.gz ./$$dir
	dir=`basename $$PWD` && ls -l ../$$dir.tar.gz

counting-test: counting-test.c 
	${CC} ${CFLAGS} -rdynamic -o $@ $<


