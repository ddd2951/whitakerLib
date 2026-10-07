#!/bin/sh
symbols=$(nm -D --undefined-only "$1") || exit 1
! printf '%s\n' "$symbols" | grep -E ' U (open|openat|creat|fopen|freopen|fdopen|mmap|dlopen)(64)?(@|$)|ifstream|filebuf|fstream'
