CC=gcc

dbmake: db.c
	${CC} -o db db.c
