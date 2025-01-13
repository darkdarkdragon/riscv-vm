# Results

## Towers benchmark 028 disks, preallocate off

Mac:
port:
23.63s user 0.11s system 96% cpu 24.576 total

optimized:
19.30s user 0.05s system 99% cpu 19.471 total
2.141784 nanosec/inst

AWS t3.micro (Intel AVX-512):

port:
real	1m21.232s

optimized
real	1m3.112s
8.332461 nanosec/inst

optimized2
real	0m55.116s
7.072318 nanosec/inst


spike
real	1m6.967s

## median (with datasize=131072 repeat=1000)

Mac:
port:
4.96s user 0.02s system 99% cpu 5.000 total
optimized:
4.13s user 0.02s system 99% cpu 4.180 total
2.724208 nanosec/inst

AWS t3.micro (Intel AVX-512):

port:
real	0m11.631s

optimized
real	0m8.909s
6.370830 nanosec/inst

optimized2
real	0m7.077s
5.137839 nanosec/inst

spike
real	2m4.947s


## prime 1000000

Mac:
port:
14.51s user 0.04s system 99% cpu 14.647 total
optimized:
10.17s user 0.04s system 99% cpu 10.311 total
2.085813 nanosec/instruction

AWS t3.micro (Intel AVX-512):

port:
real	0m38.966s

optimized
real	0m27.662s
5.920761 nanosec/inst
or
real	0m23.771s
5.377836 nanosec/inst


optimized2
real	0m23.075s
5.220222 nanosec/inst
or
real	0m21.499s
4.863649 nanosec/inst

spike
real	0m30.015s


### prime 4000000
Mac
optimized1
81.12s user
2.278890 nanosec/inst
optimized4
77.78s
2.174977 nanosec/inst
4.5% faster than optimized 1



AWS t3.micro (Intel AVX-512):

optimized
real	3m17.254s
5.478282 nanosec/inst
real	3m19.449s
5.539263 nanosec/inst

optimized2
real	2m52.090s
4.779396 nanosec/inst

optimized4
real	2m52.972s
4.803885 nanosec/inst
13% faster than optimized 1
