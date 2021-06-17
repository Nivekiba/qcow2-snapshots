#!/bin/bash

size=$1

mkdir -p disk

virt-builder ubuntu-18.04 --format qcow2 --root-password password:a \
    -o disk/ub-18.04_$size.qcow2 \
    --install fio --install qemu-guest-agent --install openssh-server \
    --hostname=box \
    --size $size \
    --copy-in fio-configs/fio.job:/root/ \
    --copy-in fio-configs/read-dev.job:/root/ \
    --copy-in fio-configs/write.job:/root/ \
    --ssh-inject root:file:$PWD/keys/id_rsa.pub \
    --run-command 'sed -i "s/GRUB_CMDLINE_LINUX_DEFAULT.*/GRUB_CMDLINE_LINUX_DEFAULT=\"console=ttyS0\"/" /etc/default/grub && update-grub' \
    --run-command 'sed -i "s/.*PermitRootLogin.*/PermitRootLogin yes/" /etc/ssh/sshd_config' \
    --run-command 'cd /root && cp fio.job fio-tmp.job && sed -i "2icreate_only=1" fio-tmp.job && fio fio-tmp.job && rm fio-tmp.job' \
    --append-line /etc/rc.local:#!/bin/bash --append-line /etc/rc.local:dhclient \
    --chmod 777:/etc/rc.local

