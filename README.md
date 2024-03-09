# Shared Memory


Basic Implementation of Shared Memory


Dependencies:
```
sudo apt-get install libuv
```


Build instruction:
```
cmake .
make
```

Test:

```
rohit@rohit:~/development/.personal/shared_memory$ ./prod
Started SHM producer: /shm_example
Started eventfd for notification with fd 10
Write SHM Header: PID: 21429 eventfd: 10
Please enter integer data to be written in shared memory.
123456
Write Data: 123456
12345678
Write Data: 12345678
1234567
Write Data: 1234567
98765
Write Data: 98765
4567
Write Data: 4567
4567
Write Data: 4567
q

rohit@rohit:~/development/.personal/shared_memory$ sudo ./cons
Connected to shared memory: /shm_example
Started eventfd notification callback on prod fd: 10 copy fd:5
Read Data : 123456
Read Data : 12345678
Read Data : 1234567
Read Data : 98765
Read Data : 4567
Read Data : 4567

```
