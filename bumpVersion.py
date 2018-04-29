import os
from git import Repo
import urllib.request
import time
import hashlib

def md5(fname):
    hash_md5 = hashlib.md5()
    with open(fname, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()

def bump_feed(path, version, md5_hash):
	newText = ''
	with open(path) as f:
		content = f.readlines()
		f.close()
	for line in content:
		if 'PKG_VERSION:=' == line[0:13]:
			newText = newText + 'PKG_VERSION:=1.' + str(version) + '\n'
		elif 'PKG_MD5SUM:=' == line [0:12]:
			newText = newText + 'PKG_MD5SUM:=' + md5_hash + '\n'
		else:
			newText = newText + line
	with open(path, "w") as f:
		f.write(newText)
		f.close()
		
def bump_openwrt(path, commit_hash):
	newText = ''
	with open(path) as f:
		content = f.readlines()
		f.close()
	for line in content:
		if 'src-git openc2x' == line[0:15]:
			print("hi")
			newText = newText + 'src-git openc2x https://github.com/fynnh/OpenC2X-feed-test.git^'+ commit_hash + '\n'
		else:
			newText = newText + line
	with open(path, "w") as f:
		f.write(newText)
		f.close()
		
def commit_file(repo, path, tagName, oldversion, version):
	origin = repo.remotes.origin
	index = repo.index
	index.add([path]) 
	index.commit('Bump version from 1.'+str(old_version)+' to 1.'+str(version))
	origin.push()
	repo.create_tag(tagName)
	origin.push(tagName)

time_string = str(time.time())

cMakeFile = 'CMakeLists.txt'
embedded_dir = '/tmp/openc2x-embedded' + time_string
feed_dir = '/tmp/openc2x-feed' + time_string
openc2x_archive = '/tmp/openc2x' + time_string + '.tar.gz'
with open(cMakeFile) as f:
    content = f.readlines()
    f.close()

newText=""
old_version = 0
version = 0
for line in content:
	if 'SET(PROJECT_VERSION_MINOR' in line:
		version_string = line[line.index('"')+1:line.rindex('"')]
		version = int(version_string)
		old_version = version
		version = version + 1
		newText = newText + 'SET(PROJECT_VERSION_MINOR "'+str(version)+'")\n'
	else:
		newText = newText + line

repo = Repo(os.path.dirname(os.path.realpath(__file__)))
if not repo.active_branch.name == "master":
	print("Not on master branch")
	exit(1)	
origin = repo.remotes.origin
remote = origin.url
base_url = remote[0:remote.index("/")]
commit = repo.head.commit
print("Do you want to release a new version with following commit to " + remote)
print('=======================')
print(commit.message)
print('=======================')
print('and bump the version from 1.'+str(old_version)+' to 1.'+str(version)+' ?[Y/n]')

tagName = "v1."+str(version)

release_url = "https://" + remote[4:-4].replace(":", "/") + "/archive/" + tagName + ".tar.gz"
# print(release_url)

choice = input()
if choice == "Y":
	with open(cMakeFile,"w") as f:
		f.write(newText)
	commit_file(repo, cMakeFile, tagName, old_version, version)
	embedded_repo = Repo.clone_from(base_url + "/OpenC2X-embedded.git", embedded_dir)
	feed_repo = Repo.clone_from(base_url + "/OpenC2X-feed.git", feed_dir)
	
	urllib.request.urlretrieve(release_url, openc2x_archive)
	md5_hash = md5(openc2x_archive)
	
	bump_feed(feed_dir + "/openc2x/Makefile", version, md5_hash)
	commit_file(feed_repo, feed_dir + "/openc2x/Makefile", tagName, old_version, version)
	feed_commit = feed_repo.head.commit
	bump_openwrt(embedded_dir + "/feeds.conf.default", feed_commit.hexsha)
	commit_file(embedded_repo, embedded_dir + "/feeds.conf.default", tagName, old_version, version)
