# Quick guide on creating system call (Debian)

**After a brief review of how our system calls work, this tutorial describes step by step how to add your own system call to the kernel and test it. There are many tutorials available so I do a detailed compilation of the steps that worked for me in my specific setup.** All the files mentioned in this tutorial are available for your reference in [this repo](https://github.com/mitsuhou/Deadlock-Detection-System-Call-Debian).

### Contents
- [Intro to syscalls](#intro-to-syscalls)
- [Hands-on](#hands-on)
    + [1. Set your virtual machine](#1-set-your-virtual-machine)
    + [2. Get the kernel source code](#2-get-the-kernel-source-code)
    + [3. Config the kernel](#3-config-the-kernel)
    + [4. Compile & install the kernel](#4-compile---install-the-kernel)
    + [5. Reboot machine with custom kernel](#5-reboot-machine-with-custom-kernel)
    + [6. Test the custom syscall](#6-test-the-custom-syscall)
- [References](#references)




# Intro to syscalls
The **kernel** is the core code of the operating system. Usually when the computer starts the bootloader and the kernel are the first pieces of code to be loaded into the machine. The kernel has control over the entire system, its memory, peripheral devices, processor and so on.

<img width="2702" height="1103" alt="Untitled" src="https://github.com/user-attachments/assets/f53fcc8a-84f2-4d0e-97b3-fac7dc2cc9f5" />

User programs and applications can utilize the resources that the operating system makes available through **system calls**. As described in the image, this system calls (syscalls) are listed in the **syscall table** and are executed in the privileged space of the **kernel mode**. Some C libraries require interaction with the kernel, thus making use of the kernel's system calls.

With the basics covered we will write a simple function in C that checks if parenthesis and braces are correctly used in an algebraic expression, and add this function to our customized kernel's syscall table.


# Hands-on
### 1. Set your virtual machine
I used Oracle's VirtualBox and [this Debian 13.5.0 machine](https://cdimage.debian.org/debian-cd/current/amd64/iso-cd/) to set up my environment.

For compiling times to be shorter, I suggest knowing how many logical cores your VM has. Configure your VM's resources while it's powered off and give it as much cores as you wish. As shown in the picture, I gave mine 8 cores. 

<img width="1974" height="1378" alt="Untitled1" src="https://github.com/user-attachments/assets/4333b77d-a594-4e5f-aeda-c1f52487ff83" />

Now power on the VM. Keep everything up do date:

```
sudo -i
apt update && apt upgrade -y
``` 

And install packages useful for kernel compiling:

```
sudo apt install build-essential bc bison flex libssl-dev libelf-dev libncurses-dev fakeroot
```

### 2. Get the kernel source code
Check your Linux kernel's version using `uname -r` (mine is `6.12.90`). Now search in  [this](https://mirrors.edge.kernel.org/pub/linux/kernel/)  site for the `.xz` file corresponding to the closest version to your kernel and download it to your home `~/` folder. In my case, the closest version was `linux-6.12.90.tar.xz`, so my command looked like this:

```
curl -O -J https://www.kernel.org/pub/linux/kernel/v6.x/linux-6.12.90.tar.xz
``` 
Now unpack the file

```
tar xvf linux-6.12.90.tar.xz
cd linux-6.12.90
``` 



### 3. Config the kernel
The Linux Kernel is extraordinarily configurable; you can enable and disable many of its features, as well as set build parameters. To make every configuration choice manually, it'll take all day. Instead, you can skip this step by simply copying your kernel’s existing configuration, which is conveniently stored in the compressed file /boot/config-6.12.90+deb13.1-amd64

```
cp /boot/config-6.12.90+deb13.1-amd64 .config
```

Let's change the kernel's name in this `.config` file. My kernel's name will be `MITSUHOU` but you can use anything meaningful to your purpose. This is a 1k+ lines file but the `CONFIG_LOCALVERSION` variable is near the top, under the general configuration.

```
CONFIG_LOCALVERSION="-MITSUHOU"
```
It is time to add our custom syscall to the syscall table. Its location depends on the machine's architecture. For x86_64, it is located at `arch/x86/entry/syscalls/syscall_64.tbl`

Now find the line with the last `common` type syscall. In the line beneath, making sure you are not using any number advised against in the table's comments, add your syscall. In my table the last common syscall was #547, so my `syscall_64.tbl` file looks something like this after adding four custom syscalls:

```
546	x32	preadv2			compat_sys_preadv64v2
547	x32	pwritev2		compat_sys_pwritev64v2
548	common	register_resource	sys_register_resource
549	common	request_resource	sys_request_resource
550	common	release_resource	sys_release_resource
551	common	detect_deadlock		sys_detect_deadlock
# This is the end of the legacy x32 range.  Numbers 548 and above are
# not special and are not to be used for x32-specific syscalls.
``` 

Lastly, we add the syscalls themselves somewhere inside the `kernel/sys.c` file and we are ready to compile.


```c
// This code is only for demonstration.
SYSCALL_DEFINE1(register_resource, int, resource_id)
{
    struct resource_node *res, *tmp;

    mutex_lock(&resource_mutex);

    list_for_each_entry(tmp, &resource_list, list) {
        if (tmp->id == resource_id) {
            mutex_unlock(&resource_mutex);
            return -EEXIST;
        }
    }

    res = kmalloc(sizeof(struct resource_node), GFP_KERNEL);
    if (!res) {
        mutex_unlock(&resource_mutex);
        return -ENOMEM;
    }

    res->id = resource_id;
    res->owner = 0;
    init_waitqueue_head(&res->wait_queue);
    INIT_LIST_HEAD(&res->waiters_list);
    
    list_add_tail(&res->list, &resource_list);

    mutex_unlock(&resource_mutex);
    return 0;
}
``` 
If you have any external library which your implemented syscall requires, you must add these library to the same folder as `kernel/sys.c` and declare them in `MAKEFILE` file.

### 4. Compile & install the kernel
Run the following script
```
# Change this if you'd like. It has no relation
# to the suffix set in the kernel config.
SUFFIX="-MITSUHOU"

# Compile the kernel
make j"$(nproc)"
# Compile and install modules
make modules_install
# Install the kernel to bootloader
make install
``` 
It will take some time to compile and install everything. Read the compiling error messages with attention.

### 5. Reboot machine with custom kernel
After successfully running the deployment script, reboot the system. Choose advanced options when loading the kernel and choose your custom kernel. In this example it is `Linux 6.12.90-MITSUHOU`.

<img width="2819" height="1360" alt="Untitled2" src="https://github.com/user-attachments/assets/db33dc0d-d1a3-4ea1-9860-4ab37da6b5fc" />

<img width="2798" height="1356" alt="Untitled3" src="https://github.com/user-attachments/assets/80c14f64-8112-4dc2-ad24-d8efad65ca59" />

To check which kernel the operating system is using:
```
uname -r
``` 

### 6. Test the custom syscall
Testing the custom syscalls is simple. Create a user program in your home or any folder of preference, have it use any of the custom syscalls, compile it using `gcc` and execute it. All the demo files are included in this repo.

# References
- **Very educational and complete tutorial (uses Arch VM instead of Ubuntu), delves into the conceptual foundations behind each step.** *Write a System Call*, by Stephen Brennan (2016). [https://brennan.io/2016/11/14/kernel-dev-ep3/](https://brennan.io/2016/11/14/kernel-dev-ep3/)
