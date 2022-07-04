#filename="./atom_log"
#filename="./build/log"
#filename="grin_24i0013_log"
#filename="grin_24c0017_log"
#filename="tokio_59c0044_log"
#filename="crossbeam_10c0012_log"
filename="../log"
f = open(filename, "r")

dic = {}

for line in f:
	log = line.split(",")
	addr = log[3]
	if addr in dic:
		dic[addr].append(log)
	else:
		dic[addr] = [log]

'''
n=10
for i, key in enumerate(dic.keys()):
	if i==n:
		break
	print(dic[key])
'''

for key in dic.keys():
	arr = dic[key]

	# check whether more than two threads access the address
	first_thread = arr[0][0];
	pr = False
	for a in arr:
		if(a[0]!=first_thread):
			pr = True
	if(pr):
		print("address: [" + key + "]")
		for a in arr:
			print("TID: " + a[0] + " OP: " + a[4])
		print("-------------------------------------------------------")
