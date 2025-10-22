# Usage
```
Usage: ./main <route_file> [lookup_file] [delete_file]
  <route_file>  : prefixes & nexthops input
  [lookup_file] : run basic test with lookups; if omitted, run performance test
  [delete_file] : run basic test with deletions additionally
```

### `valgrind`
```
$ make main-valgrind
valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./main tests/rib.simple.0000.ipv4.txt tests/lookup.address.ipv4.txt
==261574== Memcheck, a memory error detector
==261574== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
==261574== Using Valgrind-3.18.1 and LibVEX; rerun with -h for copyright info
==261574== Command: ./main tests/rib.simple.0000.ipv4.txt tests/lookup.address.ipv4.txt
==261574==
Total 15 routes added
Running basic test with lookup file tests/lookup.address.ipv4.txt...
+ Found route for 192.168.1.10    : 10.0.0.1
+ Found route for 192.168.2.10    : 10.0.0.2
+ Found route for 192.168.3.10    : 10.0.0.2
+ Found route for 1.1.1.1         : 10.5.5.2
+ Found route for 10.0.0.1        : 10.1.1.1
+ Found route for 10.64.0.1       : 10.1.1.2
+ Found route for 10.128.0.1      : 10.1.1.3
+ Found route for 10.192.0.1      : 10.1.1.3
+ Found route for 172.16.0.1      : 10.2.2.1
+ Found route for 172.16.64.1     : 10.2.2.2
+ Found route for 172.16.128.1    : 10.2.2.3
+ Found route for 203.0.113.1     : 10.3.3.1
+ Found route for 203.0.113.129   : 10.3.3.2
+ Found route for 203.0.113.193   : 10.3.3.3
+ Found route for 8.8.8.8         : 10.4.4.1
+ Found route for 15.255.255.255  : 10.4.4.1
+ Found route for 128.0.0.1       : 10.4.4.2
+ Found route for 255.255.255.255 : 10.4.4.2
+ Found route for 198.51.100.42   : 10.5.5.1
+ Found route for 198.51.100.43   : 10.4.4.2
+ Found route for 0.0.0.0         : 10.5.5.2
+ Found route for 127.0.0.1       : 10.5.5.2
==261574==
==261574== HEAP SUMMARY:
==261574==     in use at exit: 0 bytes in 0 blocks
==261574==   total heap usage: 731 allocs, 731 frees, 392,968 bytes allocated
==261574==
==261574== All heap blocks were freed -- no leaks are possible
==261574==
==261574== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

### `debug`
```
$ make debug
make BUILDTYPE=debug
make[1]: Entering directory '/home/yykzm/cronos/training/routing_table'
gcc -Wall -Wextra -g -DDEBUG -O0 -c main.c -o main.o
gcc -Wall -Wextra -g -DDEBUG -O0 -c radix.c -o radix.o
gcc -Wall -Wextra -g -DDEBUG -O0 -c test.c -o test.o
gcc -Wall -Wextra -g -DDEBUG -O0 -o main main.o radix.o test.o -lresolv
make[1]: Leaving directory '/home/yykzm/cronos/training/routing_table'
$ ./main a.txt b.txt
----- Add 1: key=64.0.0.0/3, data=0xa000005 -----
set leaf plen=3, data=0xa000005, depth=6
set leaf plen=3, data=0xa000005, depth=6
set leaf plen=3, data=0xa000005, depth=6
set leaf plen=3, data=0xa000005, depth=6
set leaf plen=3, data=0xa000005, depth=6
set leaf plen=3, data=0xa000005, depth=6
set leaf plen=3, data=0xa000005, depth=6
set leaf plen=3, data=0xa000005, depth=6
----- Add 2: key=64.0.0.0/2, data=0xa000004 -----
set leaf plen=2, data=0xa000004, depth=6
set leaf plen=2, data=0xa000004, depth=6
set leaf plen=2, data=0xa000004, depth=6
set leaf plen=2, data=0xa000004, depth=6
set leaf plen=2, data=0xa000004, depth=6
set leaf plen=2, data=0xa000004, depth=6
set leaf plen=2, data=0xa000004, depth=6
set leaf plen=2, data=0xa000004, depth=6
Total 2 routes added
Running basic test with lookup file b.txt...
+ Found route for 64.0.0.1        : 10.0.0.5
+ Found route for 96.0.0.1        : 10.0.0.4
```

# Performance

## Machine spec
- CPU: 13th Gen Intel(R) Core(TM) i9-13900H
- Mem: 31Gi

## Results

- fib.h
  ```c
  #define K 2 // change here
  #define BRANCH_SZ (1 << K)
  ```

### K=2 (4-ary)
```
$ ./main tests/rib.20251001.0000.ipv4.txt
Total 3024721 routes added
Running performance test...
Elapsed time: 15.569741 sec for 268435456 lookups
Lookup per second: 17.240843M lookups/sec
```

### K=3 (8-ary)
```
$ ./main tests/rib.20251001.0000.ipv4.txt
Total 3024721 routes added
Running performance test...
Elapsed time: 12.669105 sec for 268435456 lookups
Lookup per second: 21.188194M lookups/sec
```

### K=4 (16-ary)
```
$ ./main tests/rib.20251001.0000.ipv4.txt
Total 3024721 routes added
Running performance test...
Elapsed time: 10.876675 sec for 268435456 lookups
Lookup per second: 24.679919M lookups/sec
```

### K=5 (32-ary)
```
$ ./main tests/rib.20251001.0000.ipv4.txt
Total 3024721 routes added
Running performance test...
Elapsed time: 12.428185 sec for 268435456 lookups
Lookup per second: 21.598927M lookups/sec
```

### K=6 (64-ary)
```
$ ./main tests/rib.20251001.0000.ipv4.txt
Total 3024721 routes added
Running performance test...
Elapsed time: 11.758365 sec for 268435456 lookups
Lookup per second: 22.829318M lookups/sec
```