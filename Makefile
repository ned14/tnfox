# Used for interoperability with tools which assume that GNU make
# is the sole possible build tool in the Linux world

all:
	scons -k
debug:
	scons -k
release:
	scons -k
clean:
	scons -c
