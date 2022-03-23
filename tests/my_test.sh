#!/bin/bash
./receiver ::1 12345 &
./sender ::1 12345 -f test.txt