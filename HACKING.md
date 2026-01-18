# Setting up Clion

Some notes on debugging with Clion.

## VM Setup

Use [kvmboot](https://github.com/wrouesnel/kvmboot) to start an Ubuntu Noble
cloud boot VM.

Configure the virtual machine manager XML with the following (merge other args):

```xml
<domain xmlns:qemu="http://libvirt.org/schemas/domain/qemu/1.0" type="kvm">
    <qemu:commandline>
        <qemu:arg value="-s"/>
    </qemu:commandline>
</domain>
```

Install the debug symbol packages:

```bash
apt install -y ubuntu-dbgsym-keyring
cat < EOF | sudo tee /etc/apt/sources.list.d/ddebs.list
deb http://ddebs.ubuntu.com $(lsb_release -c) main restricted universe 
multiverse
deb http://ddebs.ubuntu.com $(lsb_release -c)-proposed main restricted universe 
multiverse
deb http://ddebs.ubuntu.com $(lsb_release -c)-updates main restricted universe 
multiverse
EOF
apt update
apt install -y linux-image-$(lsb_release -r)-generic-dbgsym
```

Update `/etc/default/grub`:

```bash
sudo sed -ri 's/GRUB_CMDLINE_LINUX="(.*)"$/GRUB_CMDLINE_LINUX="\1 nokaslr kgdboc=-ttyS0,115200"/g' /etc/default/grub
```

Reboot the VM to enable the debugger.

Then install the linux-sources:

```bash
apt install -y linux-source
```

Outside the VM download the source package:

```bash
ssh kerneldev bash -c 'cat /usr/src/linux-source-$(uname -r | cut -d'-' -f1).bz2' | sudo tar -xvf -C /usr/local/src 
```

You can now connect the debugger on port 127.0.0.1:1234 as a Remote application in clion. If the debugger isn't
starting then try `echo g | tee /proc/sysrq-trigger` in the VM.

The build source code paths for debugging symbols vary - pause and resume to get the correct name for local filepath
remapping.