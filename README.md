# Termo

# Testing
From termo directory:
`mv termo.ino termo.bak`
`mv test/* .`
Add `#define TEST 1` to top of `Termostat.h` file
Test..
`mv sensorMock* test; mv termo.ino test`
`mv termo.bak termo.ino`
Remove `#define TEST 1` from top of `Termostat.h` file