# Results

## Towers benchmark 028 disks, preallocate off

Mac:
port:
23.63s user 0.11s system 96% cpu 24.576 total

optimized:
19.30s user 0.05s system 99% cpu 19.471 total

AWS t3.micro (Intel AVX-512):

port:
real	1m21.232s

optimized
real	0m55.086s

spike
real	1m6.967s

## median (with datasize=131072 repeat=1000)

Mac:
port:
4.96s user 0.02s system 99% cpu 5.000 total
optimized:
4.13s user 0.02s system 99% cpu 4.180 total

AWS t3.micro (Intel AVX-512):

port:
real	0m11.631s

optimized
real	0m8.909s

spike
real	2m4.947s


## prime 1000000

Mac:
port:
14.51s user 0.04s system 99% cpu 14.647 total
optimized:
10.17s user 0.04s system 99% cpu 10.311 total

AWS t3.micro (Intel AVX-512):

port:
real	0m38.966s

optimized
real	0m27.662s

spike
real	0m30.015s
