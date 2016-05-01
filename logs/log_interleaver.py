import os

files = []

for (dirpath, dirnames, filenames) in os.walk("."):
    files.extend(filenames)
    break

logfiles = []

for f in files:
    if f[-4:] == ".log":
        logfiles.append(f[0:-4].split('_')) #[type,module,date,time]


#get the 5 most recend files TODO: set number of files via arg
def getTimePlusDate(logfile):
    return logfile[2]+logfile[3]

logfiles.sort(key=getTimePlusDate,reverse=True)

recentLogs = logfiles[:5]


def parseLog(log):
    d = log.split(None,2) #[module name,date,msg]
    return (d[1],log)
    
class logfileReader:
    '''read file like its a queue'''
    def __init__(self, logfile):        
        self.f = open('_'.join(logfile)+".log","r")
        self.readNewLine()

    def readNewLine(self):
        while(True):
            line = self.f.readline()
            if(line==''): #EOF
                self.logLine=[None,None]
                break
            elif(line.strip()==''): #whiteline
                continue
            else:
                self.logLine = parseLog(line)
                break

    def peekDate(self):
        return self.logLine[0]

    def popLog(self):
        log = self.logLine[1]
        self.readNewLine()
        return log


#open files with readers
logReaders = []

for logFile in recentLogs:
    logReaders.append(logfileReader(logFile))

outLog = open("log_All_"+recentLogs[0][2]+".interLog","w")


#fill output
while(len(logReaders) > 0):
    logReaders.sort(key=logfileReader.peekDate)
    outLog.write(logReaders[0].popLog())
    #remove finshed files
    if(logReaders[0].peekDate() == None):
        logReaders[0].f.close()
        logReaders=logReaders[1:]
        

outLog.close()





