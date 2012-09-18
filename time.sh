#! /bin/bash

date +"#define COMPILE_HOUR %H" | sed 's/^0//' > compiletime.h
date +"#define COMPILE_MIN %M"  | sed 's/^0//' >> compiletime.h
date +"#define COMPILE_SEC %S"  | sed 's/^0//' >> compiletime.h

