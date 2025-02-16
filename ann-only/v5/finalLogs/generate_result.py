import os

params=set()
datasets=set()
for fileName in os.listdir():
    with open(fileName, "r") as file:
        if not 'generate_result.py' in fileName:
            print(fileName)
            lines = file.readlines()
            for line in lines:
                print(line)
                if 'Command being timed' in line:
                    param=line.split('l2 ')[1]
                    params.add(param.replace('\n',''))
                    dataset=line.split('/similarity-search/dataset/')[1].split(' ')[0]
                    datasets.add(dataset)
print(len(params))
print(len(datasets))
print(params)
print(datasets)
            