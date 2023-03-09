#!/bin/bash

nodes_id=(236 223 242 244 240 205 230 227 224 219)
nodes=()
for id in ${nodes_id[@]}; do
        nodes+=(amd$id.utah.cloudlab.us)
done

function wait_status()
{
	target=$1
	message=$2
	while [ true ]
	do
		tmux capture-pane -p -t $target | grep -E "$message"
		if [ $? -eq 0 ]
		then
			break
		fi
		sleep 1
	done
}

function launch_vm()
{
	rootd=$1
	qemud=$2
	file=$3
	echo "Launch VM"
	echo "login loading..."
	tmux new -s launch_sess -d "./launch-global.sh $rootd $qemud $file"
	wait_status launch_sess "login:"
	tmux send-key -t launch_sess "root" enter
	wait_status launch_sess "Password:"
	tmux send-key -t launch_sess "a" enter
}

function gen_chain()
{
	length=$1
	size_per_snap=$2
	snapd=$3
	echo "Start chain of length $length generation with writing of $size_per_snap MB per snap"
	wait_status launch_sess "root@box:~#"
	for node in ${nodes[@]}; do
		tmux new -d "ssh -o StrictHostKeyChecking=no root@$node sar -n DEV -u -db 1 > /users/nivekiba/sarcreate$node"
	done
	tmux new -s gen_sess -d "./snap_with_write.sh $length $size_per_snap $snapd"
	echo "End of generation"
}


# root@box:~# (logged in message in vm)
launch_vm /home/nivek/Workspace/qcow2-snapshots qemu-hack-direct-access disk/ub-18.04_30G.qcow2
gen_chain 10 2560 disk/snapshots/
