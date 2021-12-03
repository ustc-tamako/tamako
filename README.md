# Tamako

## Start with Bochs

```shell
make
make bochs
```

## Start with Qemu

```shell
make
make qemu
```

## GDB in Bochs

### 1. Download bochs source code

```shell
wget https://nchc.dl.sourceforge.net/project/bochs/bochs/2.6.11/bochs-2.6.11.tar.gz
tar -xvf bochs-2.6.11.tar.gz
```
### 2. Compile bochs with gdbstub

```shell
cd bochs-2.6.11
./configure --prefix=/opt/bochs/gdbstub --enable-plugins --enable-disasm --enable-gdb-stub 
make
make install
sudo cp /opt/bochs/gdbstub/bin/bochs /usr/bin/bochs-gdb
```

### 3. Start

```shell
make
make dbochs
```

## GDB in Qemu

```shell
make
make dqemu
```