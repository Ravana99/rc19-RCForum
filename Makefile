all: user FS

user: client/user.c
	gcc -Wall -g client/user.c -o user

FS: server/FS.c
	gcc -Wall -g server/FS.c -o FS

clean:
	rm user FS
