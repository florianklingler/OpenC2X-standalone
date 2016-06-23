import os

interleave_last_x_files = 5


files = []

for (dirpath, dirnames, filenames) in os.walk("."):
    files.extend(filenames)
    break

logfiles = []

for f in files:
    if f[-4:] == ".log":
        logfiles.append(f[0:-4].split('_')) #[type,module,date,time]


#get most recend files TODO: set number of files via arg
def getTimePlusDate(logfile):
    return logfile[2]+logfile[3]

logfiles.sort(key=getTimePlusDate,reverse=True)

recentLogs = logfiles[:interleave_last_x_files]


def parseLog(log):
    d = log.split(None,2) #[module name,date,msg]
    return (d[1],log)
    
class logfileReader:
    '''read file like its a queue'''

    def __init__(self, logfile):
        fname = '_'.join(logfile)+".log"
        if not os.path.getsize(fname) > 0: # file empty
            return
        self.f = open(fname,"r")
        self.readNewLine()

    def readNewLine(self):
        line = '';
        while(line[-3:] != "\t\t\n"):
            line += self.f.readline()
            if(line==''): #EOF
                self.logLine=[None,None]
                return
        self.logLine = parseLog(line)

    def peekDate(self):
        return self.logLine[0]

    def popLog(self):
        log = self.logLine[1]
        self.readNewLine()
        return log


#open files with readers
logReaders = []

for logFile in recentLogs:
    lr = logfileReader(logFile)
    if hasattr(lr,'f'): #file opened => not empty
        logReaders.append(lr)



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





