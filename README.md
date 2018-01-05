## Virtual Machine


Build and run:

```bash
# for ARM
mkdir build && cd build
cmake ..
make
cd ..
qemu-arm -L /usr/arm-linux-gnueabi/ VM

# for x86/x64
mkdir build && cd build
cmake -D BUILD_ARM=OFF ..
make
cd ..
./VM
```
