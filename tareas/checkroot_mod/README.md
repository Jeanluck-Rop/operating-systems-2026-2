# Check-Root Module

Kernel module that creates a /proc/checkroot entry.
When someone reads it using cat, the module checks the credentials of the calling process. It retrieves the UID (User ID), the real identity of the user, and the EUID (Effective User ID), which the kernel uses to determine permissions.

Based on the EUID, it decides whether the process has root privileges or not. It then prints the UID and EUID to user space and also logs the access type in dmesg.

Overall, it’s a simple module that demonstrates how the Linux kernel handles process credentials and privilege checking.


## Kernel Tools Used
+ `current_cred()`
+ `UID / EUID`
+ `kuid_t`
+ `proc_create()`
+ `struct proc_ops`
+ `remove_proc_entry()`
+ `simple_read_from_buffer()`
+ `printk()`


## Requirements
```
sudo dnf install kernel-devel kernel-headers
```

## Build

Compile the module using:
```
make
```

Expected output (kernel version may vary):
```
make -C /lib/modules/6.14.5-100.fc40.x86_64/build M=path/check-root_mod modules
make[1]: Entering directory '/usr/src/kernels/6.14.5-100.fc40.x86_64'
make[2]: Entering directory 'path/check-root_mod'
  CC [M]  checkrootmod.o
  MODPOST Module.symvers
  CC [M]  checkrootmod.mod.o
  CC [M]  .module-common.o
  LD [M]  checkrootmod.ko
  BTF [M] checkrootmod.ko
Skipping BTF generation for checkrootmod.ko due to unavailability of vmlinux
make[2]: Leaving directory 'path/check-root_mod'
make[1]: Leaving directory '/usr/src/kernels/6.14.5-100.fc40.x86_64'
```

This generates the file:
```
checkrootmod.ko
```

## Load the Module

Insert the module into the kernel:
```
sudo insmod checkrootmod.ko
```

Verify that it is loaded:
```
lsmod | grep checkrootmod
```

Example output:
```
checkrootmod           12288  0
```

## Test the Module

Read the /proc entry as root:
```
sudo cat /proc/checkroot
```

Example output:
```
You are root :D
UID: 0
EUID: 0
```

Read it as a normal user:
```
cat /proc/checkroot
```

Example output:
```
You are not root :P
UID: 1000
EUID: 1000
```

## Check Kernel Logs

View kernel messages related to the module:
```
sudo dmesg | tail
```

Example output:
```
...
[51802.650785] Loading Kernel Module∖n
[51827.460036] checkroot: ROOT access
[51827.460077] checkroot: ROOT access
[51839.274316] checkroot: USER access
[51839.274394] checkroot: USER access
```

## Unload the Module
Remove the module from the kernel:
```
sudo rmmod checkrootmod
```

You can confirm removal with:
```
lsmod | grep checkrootmod
```
