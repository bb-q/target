#!/usr/bin/sh

gcc -lm -lSDLmain -lSDL -lcairo src/target.c src/rabbit.c -o target "$@"
