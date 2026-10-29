CC=gcc
CFLAGS=-g3 -O0

build: checkpoint readckpt restart

# Tell the linker to load restart at an address that's unlikely to conflict
# with the addresses of the process to restore
restart: restart.c
	gcc -static \
	-Wl,-Ttext-segment=5000000 -Wl,-Tdata=5100000 -Wl,-Tbss=5200000 \
        -g3 -o restart restart.c

checkpoint: ckpt.c libckpt.so
	${CC} ${CFLAGS} -o $@ $<

libckpt.so: libckpt.c
	${CC} ${CFLAGS} -shared -fPIC -o $@ $<

readckpt: readckpt.c
	${CC} ${CFLAGS} -o $@ $<

check: clean build counting-test
	rm -f myckpt					# Remove any older 'myckpt' files
	./checkpoint ./counting-test 10 &		# Start the target program in the background
	sleep 2
	cat /proc/`pgrep -n counting-test`/maps		# Print memory segment of target process
	kill -12 `pgrep -n counting-test`		# Send SIGUSR2
	sleep 2
	pkill -9 counting-test				# Terminate target process 
	./readckpt myckpt				# Inspect the checkpoint image
	./restart

clean:
	rm -f myckpt checkpoint libckpt.so readckpt counting-test restart

dist: clean
	dir=`basename $$PWD` && cd .. && tar czvf $$dir.tar.gz ./$$dir
	dir=`basename $$PWD` && ls -l ../$$dir.tar.gz

counting-test: counting-test.c 
	${CC} ${CFLAGS} -rdynamic -o $@ $<


