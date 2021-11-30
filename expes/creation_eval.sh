#!/bin/bash

disk_sizes="50G 100G 150G 200G"

rm time_creation

for d in $disk_sizes; do

echo $d >> time_creation

(
        cd /tmp/evals_test
        /tmp/qcow2-snapshots/expes/create-vm.sh $d
)
echo "vanilla" >> time_creation
/tmp/qcow2-snapshots/expes/gen_uniform_snapshot.sh 1 /tmp/qcow2-snapshots/qemu-4.2-vanilla/build /tmp/evals_test /tmp/evals_test/disk $d
/tmp/qcow2-snapshots/expes/gen_uniform_snapshot.sh 1 /tmp/qcow2-snapshots/qemu-4.2-vanilla/build /tmp/evals_test /tmp/evals_test/disk $d

echo "direct-access" >> time_creation
/tmp/qcow2-snapshots/expes/gen_uniform_snapshot.sh 1 /tmp/qcow2-snapshots/qemu-hack-direct-access/build /tmp/evals_test /tmp/evals_test/disk $d
/tmp/qcow2-snapshots/expes/gen_uniform_snapshot.sh 1 /tmp/qcow2-snapshots/qemu-hack-direct-access/build /tmp/evals_test /tmp/evals_test/disk $d

(
        cd /tmp/evals_test
        rm disk snap* -rf
)
done
