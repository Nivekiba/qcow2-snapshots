from parse import *

fil=open("file2", "r")

mr=dict()
jh=dict()

for line in fil.readlines():
	offset, l1_ind, l2_ind, ind, off2 = parse("{:d} offset, {:d} l1_index, {:d} l2_slice_index, {:d} ind, {:d} offset2\n", line)

	if str(offset) not in mr.keys():
		mr[str(offset)] = set([str(l1_ind)+"_"+str(l2_ind)])
	else:
		mr[str(offset)].add(str(l1_ind)+"_"+str(l2_ind))
		if len(mr[str(offset)]) > 1:
			jh[str(offset)] = mr[str(offset)]
		
print(jh)
