import subprocess
from subprocess import PIPE
import shutil
import time
import os
from xml.dom import minidom
from xml.parsers.expat import ExpatError
import argparse
import sys
import filecmp
from datetime import date, timedelta

#relevant paths
gBuildBatchFile = 'BuildFalcorTest.bat'
gTestListFile = 'TestList.txt'
gEmailRecipientFile = 'EmailRecipients.txt'
gResultsDir = 'TestResults'
gReferenceDir = 'ReferenceResults'      

#default values
#percent
gDefaultFrameTimeMargin = 0.05
#seconds
gDefaultLoadTimeMargin = 10
#average pixel color difference
gDefaultImageCompareMargin = 0.1
#seconds
gDefaultHangTimeDuration = 1800

#dictionary of configName : exeDirPath
gConfigDirDict = {}

class TestInfo(object):
    def determineIndex(self, generateReference):
        initialFilename = self.getResultsFile()
        if generateReference:
            while os.path.isfile(self.getReferenceFile()):
                self.Index += 1
        else:
            if os.path.isdir(self.getResultsDir()):
                while os.path.isfile(self.getResultsDir() + '\\' + self.getResultsFile()):
                    self.Index += 1
        if self.Index != 0:
            overwriteMove(initialFilename, self.getResultsFile())

    def __init__(self, name, configName):
        self.Name = name
        self.ConfigName = configName
        self.LoadErrorMargin = gDefaultLoadTimeMargin
        self.FrameErrorMargin = gDefaultFrameTimeMargin
        self.Index = 0
    def getResultsFile(self):
        return self.Name + '_TestingLog_' + str(self.Index) + '.xml'
    def getResultsDir(self):
        return gResultsDir + '\\' + self.ConfigName
    def getReferenceDir(self):
        return gReferenceDir + '\\' + self.ConfigName
    def getReferenceFile(self):
        return self.getReferenceDir() + '\\' + self.getResultsFile()
    def getFullName(self):
        return self.Name + '_' + self.ConfigName + '_' + str(self.Index)
    def getTestDir(self):
        try:
            return gConfigDirDict[self.ConfigName]
        except:
            print 'Invalid config' + self.ConfigName + ' for testInfo obj named ' + self.Name
            return ''
    def getTestPath(self):
        return self.getTestDir() + '\\' + self.Name + '.exe'
    def getRenamedTestScreenshot(self, i):
        return self.getTestDir() + '\\' + self.Name + '_' + str(self.Index) + '_' + str(i) + '.png'
    def getInitialTestScreenshot(self, i): 
        return self.getTestDir() + '\\' + self.Name + '.exe.'+ str(i) + '.png'
    def getReferenceScreenshot(self, i):
        return self.getReferenceDir() + '\\' + self.Name + '_'+ str(self.Index) + '_' + str(i) + '.png'

class SystemResult(object):
    def __init__(self):
        self.Name = ''
        self.LoadTime = 0
        self.AvgFrameTime = 0
        self.RefLoadTime = 0
        self.RefAvgFrameTime = 0
        self.LoadErrorMargin = gDefaultLoadTimeMargin
        self.FrameErrorMargin = gDefaultFrameTimeMargin
        self.CompareResults = []        

class LowLevelResult(object):
    def __init__(self):
        self.Name = ''
        self.Total = 0
        self.Passed = 0
        self.Failed = 0
        self.Crashed = 0
    def add(self, other):
        self.Total += other.Total
        self.Passed += other.Passed
        self.Failed += other.Failed
        self.Crashed += other.Crashed

#Globals
gSystemResultList = []
gLowLevelResultList = []
gSkippedList = []
gFailReasonsList = []

# -1 smaller, 0 same, 1 larger
def marginCompare(result, reference, margin):
    delta = result - reference
    if abs(delta) < reference * margin:
        return 0
    elif delta > 0:
        return 1
    else:
        return -1

def isConfigValid(config):
    if (config == 'debugd3d12' or config == 'released3d12' or 
    config == 'debugd3d11' or config == 'released3d11' or
    config == 'debugGL' or config == 'releaseGL'):
        return True
    else:
        return False

def makeDirIfDoesntExist(dirPath):
    if not os.path.isdir(dirPath):
        os.makedirs(dirPath)

def overwriteMove(filename, newLocation):
    try:
        shutil.copy(filename, newLocation)
        os.remove(filename)
    except IOError, info:
        print 'Error moving ' + filename + ' to ' + newLocation + '. Exception: ', info
        return

def renameScreenshots(testInfo, numScreenshots):
    for i in range (0, numScreenshots):
        os.rename(testInfo.getInitialTestScreenshot(i), testInfo.getRenamedTestScreenshot(i))

def compareImages(resultObj, testInfo, numScreenshots):
    global gFailReasonsList
    renameScreenshots(testInfo, numScreenshots)
    for i in range(0, numScreenshots):
        testScreenshot = testInfo.getRenamedTestScreenshot(i)
        refScreenshot = testInfo.getReferenceScreenshot(i)
        outFile = testInfo.Name + str(i) + '_Compare.png'
        command = ['magick', 'compare', '-metric', 'MSE', '-compose', 'Src', '-highlight-color', 'White', 
        '-lowlight-color', 'Black', testScreenshot, refScreenshot, outFile]
        p = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
        result = p.communicate()[0]
        spaceIndex = result.find(' ')
        resultValStr = result[:spaceIndex]
        imagesDir = testInfo.getResultsDir() + '\\Images'
        try:
            resultVal = float(resultValStr)
        except:
            gFailReasonsList.append(('For test ' + testInfo.getFullName() + 
                ' failed to compare screenshot ' + testScreenshot + ' with ref ' + refScreenshot +
                ' instead of compare result, got \"' + resultValStr + '\" from larger string \"' + result + '\"'))
            makeDirIfDoesntExist(imagesDir)
            overwriteMove(testScreenshot, imagesDir)
            resultObj.CompareResults.append(-1)
            continue
        resultObj.CompareResults.append(resultVal)
        #if the images are sufficently different, save them in test results
        if resultVal > gDefaultImageCompareMargin:
            makeDirIfDoesntExist(imagesDir)
            overwriteMove(testScreenshot, imagesDir)
            overwriteMove(outFile, imagesDir)
            gFailReasonsList.append(('For test ' + testInfo.getFullName() + ', screenshot ' +
                testScreenshot + ' differs from ' + refScreenshot + ' by ' + result + 
                ' average difference per pixel. (Exceeds threshold .01)'))
        #else just delete them
        else:
            os.remove(testScreenshot)
            os.remove(outFile)

def addSystemTestReferences(testInfo, numScreenshots):
    renameScreenshots(testInfo, numScreenshots)
    refDir = testInfo.getReferenceDir()
    makeDirIfDoesntExist(refDir)
    overwriteMove(testInfo.getResultsFile(), refDir)
    for i in range(0, numScreenshots):
        overwriteMove(testInfo.getRenamedTestScreenshot(i), refDir)

def logTestSkip(testName, reason):
    global gSkippedList
    gSkippedList.append((testName, reason))

#args should be action(build, rebuild, clean), solution, config(debug, release), and optionally project
#if no project given, performs action on entire solution
def callBatchFile(batchArgs):
    numArgs = len(batchArgs)
    if numArgs == 3 or numArgs == 4:
        batchArgs.insert(0, gBuildBatchFile)
        try:
            return subprocess.call(batchArgs)
        except (WindowsError, subprocess.CalledProcessError) as info:
            print 'Error calling batch file. Exception: ', info
            return 1
    else:
        print 'Incorrect batch file call, found ' + string(numArgs) + ' in arg list :' + batchArgs.tostring()
        return 1

def buildFail(slnName, configName):
    global gFailReasonsList
    resultsDir = testInfo.getResultsDir()
    makeDirIfDoesntExist(resultsDir)
    buildLog = 'TestedSolution_BuildFailLog.txt'
    overwriteMove(buildLog, resultsDir)
    errorMsg = 'failed to build one or more projects with config ' + configName + ' of solution ' + slnName  + ". Build log written to " + buildLog
    print 'Error: ' + errorMsg
    gFailReasonsList.append(errorMsg)

def addCrash(testName):
    logTestSkip(testName, 'Unhandled Crash')

def compareLowLevelReference(testInfo, testResult):
    global gFailReasonsList
    if not os.path.isfile(testInfo.getReferenceFile()):
        gFailReasonsList.append(('For test ' + testInfo.getFullName() + 
        ', could not find reference file ' + testInfo.getReferenceFile()))
        return
    refTag = getXMLTag(testInfo.getReferenceFile(), 'Summary')
    if refTag == None: 
        gFailReasonsList.append(('For test ' + testInfo.getFullName() + 
        ', could not find reference file ' + testInfo.getReferenceFile()))
        return
    refTotal = int(refTag[0].attributes['TotalTests'].value)
    if testResult.Total != refTotal:
        gFailReasonsList.append((testInfo.getFullName() + ': Number of tests is ' +
        str(testResult.Total) + ' which does not match ' + str(refTotal) + ' in reference'))
    refPassed = int(refTag[0].attributes['PassedTests'].value)
    if testResult.Passed != refPassed:
        gFailReasonsList.append((testInfo.getFullName() + ': Number of passed tests is ' + 
        str(testResult.Passed) + ' which does not match ' + str(refPassed) + ' in reference'))
    refFailed = int(refTag[0].attributes['FailedTests'].value)
    if testResult.Failed != refFailed:
        gFailReasonsList.append((testInfo.getFullName() + ': Number of failed tests is ' + 
        str(testResult.Failed) + ' which does not match ' + str(refFailed) + ' in reference'))
    refCrashed = int(refTag[0].attributes['CrashedTests'].value)
    if testResult.Crashed != refCrashed:
        gFailReasonsList.append((testInfo.getFullName() + ': Number of crashed tests is ' +  
        str(testResult.Crashed) + ' which does not match ' + str(refCrashed) + ' in reference'))

def getXMLTag(xmlFilename, tagName):
    try:
        referenceDoc = minidom.parse(xmlFilename)
    except ExpatError:
        return None
    tag = referenceDoc.getElementsByTagName(tagName)
    if len(tag) == 0:
        return None
    else:
        return tag

def processLowLevelTest(xmlElement, testInfo):
    global gLowLevelResultList
    newResult = LowLevelResult()
    newResult.Name = testInfo.getFullName()
    newResult.Total = int(xmlElement[0].attributes['TotalTests'].value)
    newResult.Passed = int(xmlElement[0].attributes['PassedTests'].value)
    newResult.Failed = int(xmlElement[0].attributes['FailedTests'].value)
    newResult.Crashed = int(xmlElement[0].attributes['CrashedTests'].value)
    gLowLevelResultList.append(newResult)
    #result at index 0 is summary
    gLowLevelResultList[0].add(newResult)
    makeDirIfDoesntExist(testInfo.getResultsDir())
    overwriteMove(testInfo.getResultsFile(), testInfo.getResultsDir())
    compareLowLevelReference(testInfo, newResult)

def processSystemTest(xmlElement, testInfo):
    global gSystemResultList
    global gFailReasonsList
    newSysResult = SystemResult()
    newSysResult.Name = testInfo.getFullName()
    newSysResult.LoadTime = float(xmlElement[0].attributes['LoadTime'].value)
    newSysResult.AvgFrameTime = float(xmlElement[0].attributes['FrameTime'].value)
    newSysResult.LoadErrorMargin = testInfo.LoadErrorMargin 
    newSysResult.FrameErrorMargin = testInfo.FrameErrorMargin
    numScreenshots = int(xmlElement[0].attributes['NumScreenshots'].value)
    referenceFile = testInfo.getReferenceFile()
    resultFile = testInfo.getResultsFile()
    if not os.path.isfile(referenceFile):
        logTestSkip(testInfo.getFullName(), 'Could not find reference file ' + referenceFile + ' for comparison')
        return 
    refResults = getXMLTag(referenceFile, 'Summary')
    if refResults == None:
        logTestSkip(testInfo.getFullName(), 'Error getting xml data from reference file ' + referenceFile)
        return
    newSysResult.RefLoadTime = float(refResults[0].attributes['LoadTime'].value)
    newSysResult.RefAvgFrameTime = float(refResults[0].attributes['FrameTime'].value)
    #check avg fps
    if newSysResult.AvgFrameTime != 0 and newSysResult.RefAvgFrameTime != 0:
        if marginCompare(newSysResult.AvgFrameTime, newSysResult.RefAvgFrameTime, newSysResult.FrameErrorMargin) == 1:
            gFailReasonsList.append((testInfo.getFullName() + ': average frame time ' + 
            str(newSysResult.AvgFrameTime) + ' is larger than reference ' + str(newSysResult.RefAvgFrameTime) + 
            ' considering error margin ' + str(newSysResult.FrameErrorMargin * 100) + '%'))
    #check load time
    if newSysResult.LoadTime != 0 and newSysResult.RefLoadTime != 0:
        if newSysResult.LoadTime > (newSysResult.RefLoadTime + newSysResult.LoadErrorMargin):
            gFailReasonsList.append(testInfo.getFullName() + ': load time' + (str(newSysResult.LoadTime) +
            ' is larger than reference ' + str(newSysResult.RefLoadTime) + ' considering error margin ' + 
            str(newSysResult.LoadErrorMargin) + ' seconds'))
    compareImages(newSysResult, testInfo, numScreenshots)
    gSystemResultList.append(newSysResult)
    makeDirIfDoesntExist(testInfo.getResultsDir())
    overwriteMove(resultFile, testInfo.getResultsDir())

def readTestList(generateReference, buildTests):
    global gConfigDirDict
    file = open(gTestListFile)
    contents = file.read()
    slnEndIndex = contents.find(' ')
    slnName = contents[:slnEndIndex]
    slnConfigStartIndex = contents.find('{')
    slnConfigEndIndex = contents.find('}')
    configData = contents[slnConfigStartIndex + 1 : slnConfigEndIndex]
    configDataList = configData.split(' ')
    for i in xrange(0, len(configDataList), 2):
        exeDir = configDataList[i + 1]
        configName = configDataList[i].lower()
        gConfigDirDict[configName] = exeDir
        if buildTests:
            callBatchFile(['clean', slnName, configName])
            #delete bin dir
            if os.path.isdir(exeDir):
                shutil.rmtree(exeDir)
            #returns 1 on fail
            if callBatchFile(['rebuild', slnName, configName]):
                buildFail(slnName, configName)
        else:
            cleanScreenShots(exeDir)

    contents = contents[slnConfigEndIndex + 1 :]
    argStartIndex = contents.find('{')
    while(argStartIndex != -1):
        testName = cleanupString(contents[:argStartIndex])
        argEndIndex = contents.find('}')
        args = cleanupString(contents[argStartIndex + 1 : argEndIndex])
        argsList = args.split(',')
        contents = contents[argEndIndex + 1 :]
        configStartIndex = contents.find('{')
        configEndIndex = contents.find('}')
        configList = cleanupString(contents[configStartIndex + 1 : configEndIndex]).split(' ')
        #run test for each config and each set of args
        for config in configList:
            print 'Running ' + testName + ' in config ' + config 
            for argSet in argsList:
                testInfo = TestInfo(testName, config)
                runTest(testInfo, cleanupString(argSet), generateReference)

        #goto next set
        contents = contents[configEndIndex + 1 :]
        argStartIndex = contents.find('{')
    return slnName

def runTest(testInfo, cmdLine, generateReference):
    testPath = testInfo.getTestPath()
    if not os.path.exists(testPath):
        logTestSkip(testInfo.getFullName(), 'Unable to find ' + testPath)
        return
    try:
        p = subprocess.Popen(testPath + ' ' + cmdLine)
        #run test until timeout or return
        start = time.time()
        while p.returncode == None:
            p.poll()
            cur = time.time() - start
            if cur > gDefaultHangTimeDuration:
                p.kill()
                logTestSkip(testInfo.getFullName(), ('Test timed out ( > ' + 
                    str(gDefaultHangTimeDuration) + ' seconds)'))
                return
        #ensure results file exists
        if not os.path.isfile(testInfo.getResultsFile()):
            logTestSkip(testInfo.getFullName(), 'Failed to open test result file ' + testInfo.getResultsFile())
            return
        #check for name conflicts
        testInfo.determineIndex(generateReference)
        #get xml from results file
        summary = getXMLTag(testInfo.getResultsFile(), 'Summary')
        if summary == None:
            logTestSkip(testInfo.getFullName(), 'Error getting xml data from ' + testInfo.getResultsFile())
            if generateReference:
                makeDirIfDoesntExist(testInfo.getReferenceDir())
                overwriteMove(testInfo.getResultsFile(), testInfo.getReferenceDir())
            else:
                makeDirIfDoesntExist(testInfo.getResultsDir())
                overwriteMove(testInfo.getResultsFile(), testInfo.getResultsDir())
            return
        #gen system ref
        if cmdLine and generateReference:
            numScreenshots = int(summary[0].attributes['NumScreenshots'].value)
            addSystemTestReferences(testInfo, numScreenshots)
        #process system
        elif cmdLine:
            processSystemTest(summary, testInfo)
        #gen low level ref
        elif generateReference:
            makeDirIfDoesntExist(testInfo.getReferenceDir())
            overwriteMove(testInfo.getResultsFile(), testInfo.getReferenceDir())
        #process low level
        else:
            processLowLevelTest(summary, testInfo)
    except subprocess.CalledProcessError:
        addCrash(testInfo.getFullName())

def lowLevelTestResultToHTML(result):
    html = '<tr>'
    if int(result.Crashed) > 0:
        html = '<tr bgcolor="yellow">\n'
        font = '<font>'
    elif int(result.Failed) > 0:
        html = '<tr bgcolor="red">\n'
        font = '<font color="white">'
    else:
        font = '<font>'
    html += '<td>' + font + result.Name + '</font></td>\n'
    html += '<td>' + font + str(result.Total) + '</font></td>\n'
    html += '<td>' + font + str(result.Passed) + '</font></td>\n'
    html += '<td>' + font + str(result.Failed) + '</font></td>\n'
    html += '<td>' + font + str(result.Crashed) + '</font></td>\n'
    html += '</tr>\n'
    return html

def getLowLevelTestResultsTable():
    html = '<table style="width:100%" border="1">\n'
    html += '<tr>\n'
    html += '<th colspan=\'5\'>Low Level Test Results</th>\n'
    html += '</tr>\n'
    html += '<tr>\n'
    html += '<th>Test</th>\n'
    html += '<th>Total</th>\n'
    html += '<th>Passed</th>\n'
    html += '<th>Failed</th>\n'
    html += '<th>Crashed</th>\n'
    for result in gLowLevelResultList:
        html += lowLevelTestResultToHTML(result)
    html += '</table>\n'
    return html

def systemTestResultToHTML(result):
    #if missing data for both load time and frame time, no reason for table entry
    if ((result.LoadTime == 0 or result.RefLoadTime == 0) and 
        (result.AvgFrameTime == 0 or result.RefAvgFrameTime == 0)):
        return ''

    html = '<tr>'
    html += '<td>' + result.Name + '</td>\n'
    #if dont have real load time data, dont put it in table
    if result.LoadTime == 0 or result.RefLoadTime == 0:
        html += '<td></td><td></td><td></td>'
    else:
        html += '<td>' + str(result.LoadErrorMargin) + '</td>\n'
        if result.LoadTime > (result.RefLoadTime + result.LoadErrorMargin):
            html += '<td bgcolor="red"><font color="white">' 
        elif result.LoadTime < result.RefLoadTime:
            html += '<td bgcolor="green"><font color="white">' 
        else:
            html += '<td><font>' 
        html += str(result.LoadTime) + '</font></td>\n'
        html += '<td>' + str(result.RefLoadTime) + '</td>\n'

    #if dont have real frame time data, dont put it in table
    if result.AvgFrameTime == 0 or result.RefAvgFrameTime == 0:
        html += '<td></td><td></td><td></td>'
    else:
        html += '<td>' + str(result.FrameErrorMargin * 100) + '</td>\n'
        compareResult = marginCompare(result.AvgFrameTime, result.RefAvgFrameTime, result.FrameErrorMargin)
        if(compareResult == 1):
            html += '<td bgcolor="red"><font color="white">' 
        elif(compareResult == -1):
            html += '<td bgcolor="green"><font color="white">' 
        else:
            html += '<td><font>' 
        html += str(result.AvgFrameTime) + '</font></td>\n'   
        html += '<td>' + str(result.RefAvgFrameTime) + '</td>\n'

    html += '</tr>\n'
    return html

def getSystemTestResultsTable():
    html = '<table style="width:100%" border="1">\n'
    html += '<tr>\n'
    html += '<th colspan=\'7\'>System Test Results</th>\n'
    html += '</tr>\n'
    html += '<th>Test</th>\n'
    html += '<th>Load Time Error Margin Secs</th>\n'
    html += '<th>Load Time</th>\n'
    html += '<th>Ref Load Time</th>\n'
    html += '<th>Frame Time Error Margin %</th>\n'
    html += '<th>Avg Frame Time</th>\n'
    html += '<th>Ref Frame Time</th>\n'
    for result in gSystemResultList:
        html += systemTestResultToHTML(result)
    html += '</table>\n'
    return html

def getImageCompareResultsTable():
    max = 0
    #table needs max num of screenshots plus one columns 
    for result in gSystemResultList:
        if len(result.CompareResults) > max:
            max = len(result.CompareResults)
    html = '<table style="width:100%" border="1">\n'
    html += '<tr>\n'
    html += '<th colspan=\'' + str(max + 1) + '\'>Image Compare Tests</th>\n'
    html += '</tr>\n'
    html += '<th>Test</th>\n'
    for i in range (0, max):
        html += '<th>SS' + str(i) + '</th>\n'
    for result in gSystemResultList:
        if len(result.CompareResults) > 0:
            html += '<tr>\n'
            html += '<td>' + result.Name + '</td>\n'
            for compare in result.CompareResults:
                if float(compare) > gDefaultImageCompareMargin:
                    html += '<td bgcolor="red"><font color="white">' + str(compare) + '</font></td>\n'
                else:
                    html += '<td>' + str(compare) + '</td>\n'
            html += '</tr>\n'
    html += '</table>\n'
    return html

def skipToHTML(name, reason):
    html = '<tr>\n'
    html += '<td>' + name + '</td>\n'
    html += '<td>' + reason + '</td>\n'
    html += '</tr>\n'
    return html

def getSkipsTable():
    html = '<table style="width:100%" border="1">\n'
    html += '<tr>\n'
    html += '<th colspan=\'2\'>Skipped Tests</th>'
    html += '</tr>'
    html += '<th>Test</th>\n'
    html += '<th>Reason for Skip</th>\n'
    for name, reason in gSkippedList:
        html += skipToHTML(name, reason)
    html += '</table>'
    return html

def outputHTML(openSummary, slnName):
    html = getLowLevelTestResultsTable()
    html += '<br><br>'
    html += getSystemTestResultsTable()
    html += '<br><br>'
    html += getImageCompareResultsTable()
    html += '<br><br>'
    html += getSkipsTable()
    resultSummaryName = gResultsDir + '\\' + slnName + '_TestSummary.html'
    outfile = open(resultSummaryName, 'w')
    outfile.write(html)
    outfile.close()
    if(openSummary):
        os.system("start " + resultSummaryName)

def cleanScreenShots(ssDir):
    if os.path.isdir(ssDir):
        screenshots = [s for s in os.listdir(ssDir) if s.endswith('.png')]
        for s in screenshots:
            os.remove(ssDir + '\\' + s)

def sendFatalFailEmail(failMsg):
    subject = '[FATAL TESTING ERROR] '
    sendEmail(subject, failMsg, None)

def sendTestingEmail():
    todayStr = (date.today()).strftime("%m-%d-%y")
    todayFile = gResultsDir + '\\TestSummary.html'
    body = 'Attached is the testing summary for ' + todayStr
    if len(gFailReasonsList) > 0 or len(gSkippedList) > 0:
        subject = '[Different] '
        body += '.\nReasons for Difference'
        for reason in gFailReasonsList:
            body += '.\n' + reason
        for name, skip in gSkippedList:
            body += '.\n' + name + ': ' + skip
    else:
        subject = '[Unchanged] '
    subject += 'Falcor Automated Testing for ' + todayStr
    sendEmail(subject, body, todayFile)

#pass none if no attach
def sendEmail(subject, body, attachment):
    sender = 'clavelle@nvidia.com'
    recipients = str(open(gEmailRecipientFile, 'r').read());
    subprocess.call(['blat.exe', '-install', 'mail.nvidia.com', sender])
    if attachment != None:
        subprocess.call(['blat.exe', '-to', recipients, '-subject', subject, '-body', body, '-attach', attachment])
    else:
        subprocess.call(['blat.exe', '-to', recipients, '-subject', subject, '-body', body])

def main():
    global gResultsDir
    global gLowLevelResultList
    global gReferenceDir
    global gTestListFile
    parser = argparse.ArgumentParser()
    parser.add_argument('-nb', '--nobuild', action='store_true', help='run without rebuilding Falcor and test apps')
    parser.add_argument('-ne', '--noemail', action='store_true', help='run without emailing the result summary')
    parser.add_argument('-ss', '--showsummary', action='store_true', help='opens testing summary upon completion')
    parser.add_argument('-gr', '--generatereference', action='store_true', help='generates reference testing logs and images')
    parser.add_argument('-ref', '--referencedir', action='store', help='Allows user to specify an existing reference dir')
    parser.add_argument('-tests', '--testlist', action='store', help='Allows user to specify the test list file')
    args = parser.parse_args()

    if args.referencedir:
        refDir = cleanupString(args.referencedir)
        if os.path.isdir(refDir):
            gReferenceDir = refDir
        elif not args.generatereference:
            print 'Fatal Error, Failed to find user specified reference dir: ' + args.referencedir
            sys.exit(1)

    if args.generatereference:
        if not os.path.isdir(gReferenceDir):
            try:
                os.makedirs(gReferenceDir)
            except:
                print 'Fatal Error, Failed to create reference dir.'
                sys.exit(1)

    if args.testlist:
        if not os.path.exists(args.testlist):
            print 'Error, Failed to find user specified test list. using default ' + testlist
        else:
            gTestListFile = args.testlist

    #make outer dir if need to
    makeDirIfDoesntExist(gResultsDir)
    #make inner dir for this results
    dateStr = date.today().strftime("%m-%d-%y")
    gResultsDir += '\\' + dateStr
    if os.path.isdir(gResultsDir):
        #remove date subdir if exists
        shutil.rmtree(gResultsDir, ignore_errors=True)
        try:
            time.sleep(5)
            os.makedirs(gResultsDir)
        except:
            if not args.noemail:
                sendFatalFailEmail('Fatal Error, Failed to create test result folder')
                sys.exit(1)

    if not args.generatereference:
        resultSummary = LowLevelResult()
        resultSummary.Name = 'Summary'
        gLowLevelResultList.append(resultSummary)

    slnName = readTestList(args.generatereference, not args.nobuild)
    if not args.generatereference:
        outputHTML(args.showsummary, slnName)

    if not args.noemail and not args.generatereference:
        sendTestingEmail()
    
    if args.generatereference or args.noemail:
        if(len(gSkippedList) > 0 or len(gFailReasonsList) > 0):
            print '\n\nRan into the following issues ....\n-----\n'
            for name, skip in gSkippedList:
                print name + ': ' + skip
            for reason in gFailReasonsList:
                print reason

def cleanupString(string):
    string = string.replace('\t', '')
    return string.replace('\n', '').strip()

if __name__ == '__main__':
    main()
