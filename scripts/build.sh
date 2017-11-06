#!/bin/bash

cd /srv/uthreads/ 
make clean > /dev/null
make > /dev/null
./tester
