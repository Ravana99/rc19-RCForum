all: user FS

user: client/user.c
	gcc client/user.c -o user

FS: server/FS.c
	gcc server/FS.c -o FS

clean:
	rm user FS
