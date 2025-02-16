# datasets=['imageNet', 'audio', 'notre', 'uqv', 'MNIST', 'trevi', 'cifar', 'ukbench', 'glove', 'sun', 'nuswide', 'random', 'crawl', 'millionSong', 'enron', 'deep', 'astro1m', 'tiny5m', 'bigann', 'gist', 'lastfm', 'movielens', 'netflix', 'nytimes', 'sald1m', 'sift', 'space1V', 'text-to-image', 'word2vec', 'yahoomusic','ethz', 'vcseis', 'txed', 'lendb', 'stead','geofon','instancegm','Music','Yelp']
datasets=['audio','cifar','ethz','geofon','lastfm','MNIST','movielens','netflix','notre','nytimes','sun','vcseis','yahoomusic','Yelp']
file = open('finger_script.sh', "r")
# Read the entire content of the file
content = file.read()
runId=1
final_scripts=''
params=[/12,50,100),(24,50,100),(48,50,500),(12,100,400)]
for i in range(0,len(params)):
    M=str(params[i][0])
    EFS=str(params[i][1])
    EFC=str(params[i][2])
    for dataset in datasets:
        data=content.replace('[data]',dataset)
        data=data.replace('[M]',M)
        data=data.replace('[EFS]',EFS)
        data=data.replace('[EFC]',EFC)
        fn='run-'+dataset+'-'+str(runId)+'-'+str(i)+'.sh'
        with open(fn, "w") as f:
            f.write(data)
            final_scripts=final_scripts+'sbatch '+fn+' && '
final_scripts=final_scripts+'echo 1'
print(final_scripts)
