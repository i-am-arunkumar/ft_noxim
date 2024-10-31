from yaml import load, dump
try:
    from yaml import CLoader as Loader, CDumper as Dumper
except ImportError:
    from yaml import Loader, Dumper

import subprocess
import re
import json

result = {}

def store(data, name):
    print(f"started {name}")
    out = open(f"./configs/{name}.yaml","w")
    dump(data,out,Dumper=Dumper)
    out.close()
    try:
    	output = subprocess.check_output(f"noxim -config ./configs/{name}.yaml", shell=True, text=True)
    	delay_max =   re.search(r'Max delay \(cycles\): [\d\.\d]+', output) 
    	delay_avg = re.search(r'Global average delay \(cycles\): [\d\.\d]+', output)
    	throughput = re.search(r'Network throughput \(flits\/cycle\): [\d\.\d]+', output)
    	file = open(f"./outputs/{name}.txt","w")
    	result[name] = {
    		"delay_max" : float(delay_max.group().split(":")[1]),
    		"delay_avg" : float(delay_avg.group().split(":")[1]),
    		"throughput" : float(throughput.group().split(":")[1])
    	}
    	print(output, file=file)
    	file.close()
    	print(f"completed {name}")
    except:
    	print("Failed : ", f"noxim -config ./configs/{name}.yaml")



f = open("./config_examples/default_config.yaml")
data = load(f, Loader=Loader)
f.close()


dims = [50] #[4,8,16,32,50]
buffer_depths = [4,8,16,32,64, 128, 256]
winocs = [True, False]
flit_sizes = [32,64,128]


for dim in dims:
    for winoc in winocs:
        for flit_size in flit_sizes:
            data["mesh_dim_x"] = dim
            data["mesh_dim_y"] = dim
            data["use_winoc"] = winoc
            data["flit_size"] = flit_size
            if dim == 50:
                data["buffer_depth"] = 64
            else:
                data["buffer_depth"] = dim
            store(data,f"mesh{dim}x{dim}_{winoc}_{flit_size}") 

print(result)
    		
with open("results.json", "w") as outfile: 
    json.dump(result, outfile)

    		





