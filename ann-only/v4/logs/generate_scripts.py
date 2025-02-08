# datasets=['imageNet', 'audio', 'notre', 'uqv', 'MNIST', 'trevi', 'cifar', 'ukbench', 'glove', 'sun', 'nuswide', 'random', 'crawl', 'millionSong', 'enron', 'deep', 'astro1m', 'tiny5m', 'bigann', 'gist', 'lastfm', 'movielens', 'netflix', 'nytimes', 'sald1m', 'sift', 'space1V', 'text-to-image', 'word2vec', 'yahoomusic']
# datasets=['ethz', 'vcseis', 'txed', 'lendb', 'stead','geofon','instancegm','Music','Yelp']
datasets=['imageNet','millionSong','nuswide']
file = open('finger_script.sh', "r")
# Read the entire content of the file
content = file.read()

final_scripts=''

for dataset in datasets:
    data=content.replace('dataset-type',dataset)
    str='run-'+dataset+'.sh'
    with open(str, "w") as f:
        f.write(data)
        final_scripts=final_scripts+'sbatch '+str+' && '
final_scripts=final_scripts+'echo 1'
print(final_scripts)
