from fodules.excel import *
from sklearn import svm

"""
Prior to these steps you have to run the estimation and evaluation
algorithms, in order to have data to train with.
"""

# This is the merged_results.csv file we are going to train with:
training_file = '/home/angel/Desktop/20160705130908-bmtg-wav/merged_results.csv'

# we use the whole bmtg collection for training!
features = features_from_csv(training_file, 4, 76)  # cols. 3-39 contain the 36 pcp values.
targets = stringcell_from_csv(training_file, 78)    # col. 75 holds the ground-truth of the file.
filenames = stringcell_from_csv(training_file, 0)   # col. 0 stores the filename.
print len(features), 'files used for training.'

# here is the actual support vector machine.
svc = svm.SVC()
svc.fit(features, targets)


# NOW WE NEED TO LOAD DIFFERENT DATASETS TO TEST!

analysis_file = '/home/angel/Desktop/gs-wav-3x-36-bmtg3/merged_results.csv'
features = features_from_csv(analysis_file, 4, 76)  # cols. 3-39 contain the 36 pcp values.
filenames = stringcell_from_csv(analysis_file, 0)
an_folder = analysis_file[:analysis_file.rfind('/')]

for i in range(len(features)):
    prediction = svc.predict(features[i].reshape(-1, 72))
    prediction = "{0}, ".format(str(prediction)[2:-2])
    append_results = open(an_folder + '/' + filenames[i] + '.key', 'a')
    append_results.write(prediction)
    append_results.close()
