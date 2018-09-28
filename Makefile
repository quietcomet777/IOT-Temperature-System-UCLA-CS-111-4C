#!/bin/bash

#NAME: Dan Molina
#EMAIL: digzmol45@gmail.com
#ID: 104823116


.SILENT:

default: lab4c_tcp.c lab4c_tls.c
	gcc -Wall -Wextra lab4c_tcp.c -o lab4c_tcp -lmraa -lm
	gcc -Wall -Wextra lab4c_tls.c -o lab4c_tls -lmraa -lm -lcrypto -lssl

check: default



clean: default
	rm *.gz lab4c_tcp lab4c_tls

dist:
	tar -czvf lab4c-104823116.tar.gz *

