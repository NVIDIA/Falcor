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
gTestListFile = 'C:\\Users\\clavelle\\Desktop\\FalcorGitHub\\FalcorTest\\TestList.txt'
gEmailRecipientFile = 'EmailRecipients.txt'
gDebugDir = 'C:\\Users\\clavelle\\Desktop\\FalcorGitHub\\FalcorTest\\Bin\\x64\\Debug\\'
gReleaseDir = 'C:\\Users\\clavelle\\Desktop\\FalcorGitHub\\FalcorTest\\Bin\\x64\\Release\\'
gResultsDir = 'TestResults'
gReferenceDir = 'SystemTestReferenceResults'

class TestInfo(object):
    def __init__(self, name, configName, errorMargin):
        self.Name = name
        self.ConfigName = configName
        self.ErrorMargin = errorMargin
    def getResultsFile(self):
        return self.Name + '_TestingLog.xml'
    def getBuildFailFile(self):
        return self.Name + '_BuildFailLog.txt'
    def getResultsDir(self):
        return gResultsDir + '\\' + self.ConfigName
    def getReferenceDir(self):
        return gReferenceDir + '\\' + self.ConfigName
    def getReferenceFile(self):
        return self.getReferenceDir() + '\\' + self.getResultsFile()
    def getFullName(self):
        return self.Name + '_' + self.ConfigName
    def getTestDir(self):
        if (self.ConfigName == 'debugd3d12' or self.ConfigName == 'debugd3d11' or 
            self.ConfigName == 'debugGL'):
            return gDebugDir
        elif(self.ConfigName == 'released3d12' or self.ConfigName == 'released3d11' or
            self.ConfigName == 'releaseGL'):
            return gReleaseDir
        else:
            print 'Invalid config' + self.ConfigName + ' for testInfo obj named ' + self.Name
            return ''
    def getTestPath(self):
        return self.getTestDir() + '\\' + self.Name + '.exe'
    def getTestScreenshot(self, i): 
        return self.getTestDir() + '\\' + self.Name + '.exe.'+ str(i) + '.png'
    def getReferenceScreenshot(self, i):
        return self.getReferenceDir() + '\\' + self.Name + '.exe.'+ str(i) + '.png'

class SystemResult(object):
    def __init__(self):
        self.Name = ''
        self.LoadTime = 0
        self.AvgFrameTime = 0
        self.RefLoadTime = 0
        self.RefAvgFrameTime = 0
        self.ErrorMargin = 0.05
        self.CompareResults = []        

class TestResults(object):
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
gLowLevelResultSummary = TestResults()
gSkippedList = []
gGenRefResults = False

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

def compareImages(resultObj, testInfo, numScreenshots):
    for i in range(0, numScreenshots):
        testScreenshot = testInfo.getTestScreenshot(i)
        refScreenshot = testInfo.getReferenceScreenshot(i)
        outFile = testInfo.Name + str(i) + '_Compare.png'
        command = ['magick', 'compare', '-metric', 'MSE', '-compose', 'Src', '-highlight-color', 'White', 
        '-lowlight-color', 'Black', testScreenshot, refScreenshot, outFile]
        p = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
        result = p.communicate()[0]
        spaceIndex = result.find(' ')
        result = result[:spaceIndex]
        resultObj.CompareResults.append(result)
        #if the images are sufficently different, save them in test results
        if float(result) > 0.01:
            imagesDir = testInfo.getResultsDir() + '\\Images'
            makeDirIfDoesntExist(imagesDir)
            overwriteMove(testScreenshot, imagesDir)
            overwriteMove(outFile, imagesDir)
        #else just delete them
        else:
            os.remove(testScreenshot)
            os.remove(outFile)

def addSystemTestReferences(testInfo, numScreenshots):
    refDir = testInfo.getReferenceDir()
    makeDirIfDoesntExist(refDir)
    overwriteMove(testInfo.getResultsFile(), refDir)
    for i in range(0, numScreenshots):
        overwriteMove(testInfo.getTestScreenshot(i), refDir)

def logTestSkip(testName, reason):
    global gSkippedList
    gSkippedList.append((testName, reason))

#args should be action(build, rebuild, clean), config(debug, release), and optionally project
#if no project given, performs action on entire solution
def callBatchFile(batchArgs):
    numArgs = len(batchArgs)
    if numArgs == 2 or numArgs == 3:
        batchArgs.insert(0, gBuildBatchFile)
        try:
            return subprocess.call(batchArgs)
        except (WindowsError, subprocess.CalledProcessError) as info:
            print 'Error calling batch file. Exception: ', info
            return 1
    else:
        print 'Incorrect batch file call, found ' + string(numArgs) + ' in arg list :' + batchArgs.tostring()
        return 1

def buildFail(fatal, testInfo):
    resultsDir = testInfo.getResultsDir()
    makeDirIfDoesntExist(resultsDir)
    overwriteMove(testInfo.getBuildFailFile(), resultsDir)
    if fatal:
        print 'Fatal error, failed to build ' + testInfo.Name
        sys.exit(1)
    else:
        logTestSkip(testInfo.getFullName(), 'Build Failure')

#Test Base tries to catch any thrown exceptions, but it's possible that the
#program might crash in a different way
def addCrash(testName):
    logTestSkip(testName, 'Unhandled Crash')

def processLowLevelTest(xmlElement, testInfo):
    global gLowLevelResultList
    global gLowLevelResultSummary
    newResult = TestResults()
    newResult.Name = testInfo.getFullName()
    newResult.Total = int(xmlElement[0].attributes['TotalTests'].value)
    newResult.Passed = int(xmlElement[0].attributes['PassedTests'].value)
    newResult.Failed = int(xmlElement[0].attributes['FailedTests'].value)
    newResult.Crashed = int(xmlElement[0].attributes['CrashedTests'].value)
    gLowLevelResultList.append(newResult)
    gLowLevelResultSummary.add(newResult)
    overwriteMove(testInfo.getResultsFile(), testInfo.getResultsDir())

def processSystemTest(xmlElement, testInfo):
    global gSystemResultList
    newSysResult = SystemResult()
    newSysResult.Name = testInfo.getFullName()
    newSysResult.LoadTime = float(xmlElement[0].attributes['LoadTime'].value)
    newSysResult.AvgFrameTime = float(xmlElement[0].attributes['FrameTime'].value)
    newSysResult.ErrorMargin = testInfo.ErrorMargin 
    numScreenshots = int(xmlElement[0].attributes['NumScreenshots'].value)
    referenceFile = testInfo.getReferenceFile()
    resultFile = testInfo.getResultsFile()
    if os.path.isfile(referenceFile):
        try:
            referenceDoc = minidom.parse(referenceFile)
        except ExpatError:
            logTestSkip(testInfo.getFullName(), resultFile + ' was found, but is not correct xml')
            newSysResult.RefLoadTime = -1
            newSysResult.RefAvgFrameTime = -1
            overwriteMove(resultFile, testInfo.getResultsDir())
            return

        refResults = referenceDoc.getElementsByTagName('SystemResults')
        if len(refResults) != 1:
            logTestSkip(testInfo.getFullName(), referenceFile + ' was found and is XML, but is improperly formatted')
            newSysResult.RefLoadTime = -1
            newSysResult.RefAvgFrameTime = -1
        else:
            newSysResult.RefLoadTime = float(refResults[0].attributes['LoadTime'].value)
            newSysResult.RefAvgFrameTime = float(refResults[0].attributes['FrameTime'].value)
            compareImages(newSysResult, testInfo, numScreenshots)
    else:
        print 'Could not find reference file ' + referenceFile + ' for comparison'
        newSysResult.RefLoadTime = -1
        newSysResult.RefAvgFrameTime = -1
    gSystemResultList.append(newSysResult)
    overwriteMove(resultFile, testInfo.getResultsDir())

def processTestResult(testInfo):
    resultFile = testInfo.getResultsFile()
    if os.path.isfile(resultFile):
        resultDir = testInfo.getResultsDir()
        makeDirIfDoesntExist(resultDir)
        try:
            xmlDoc = minidom.parse(resultFile)
        except ExpatError:
            logTestSkip(testInfo.getFullName(), resultFile + ' was found, but is not correct xml')
            overwriteMove(resultFile, resultDir)
            return
        lowLevelResults = xmlDoc.getElementsByTagName('Summary')
        sysResults = xmlDoc.getElementsByTagName('SystemResults')
        if len(lowLevelResults) == 1:
            if gGenRefResults:
                makeDirIfDoesntExist(testInfo.getReferenceDir())
                overwriteMove(resultFile, testInfo.getReferenceDir())
            else:
                processLowLevelTest(lowLevelResults, testInfo)
        elif len(sysResults) ==  1: 
            if gGenRefResults:
                #need to know how many ss to move into ref dir
                numScreenshots = int(sysResults[0].attributes['NumScreenshots'].value)
                addSystemTestReferences(testInfo, numScreenshots)
            else:
                processSystemTest(sysResults, testInfo)
        else:
            logTestSkip(testInfo.getFullName(), resultFile + ' was found and is XML, but is improperly formatted')
            overwriteMove(resultFile, resultDir)
    else:
        logTestSkip(testInfo.Name, 'Unable to find result file ' + resultFile)

def readTestList(buildTests):
    testList = open(gTestListFile)
    for line in testList.readlines():
        #strip off newline if there is one 
        if line[-1] == '\n':
            line = line[:-1]

        spaceIndex = line.find(' ');
        if spaceIndex == -1:
            logTestSkip(line, 'Improperly formatted test request')
            continue

        testName = line[:spaceIndex]
        configName = line[spaceIndex + 1:]
        secondSpaceIndex = configName.find(' ')

        if secondSpaceIndex != -1:
            errorMarginStr = configName[secondSpaceIndex + 1:]
            try:
                errorMargin = float(errorMarginStr)
            except ValueError:
                errorMargin = 0.05
            configName = configName[:secondSpaceIndex]
        else:
            errorMargin = .05

        configName = configName.lower()
        testInfo = TestInfo(testName, configName, errorMargin)
        if isConfigValid(configName):
            if buildTests:
                if callBatchFile(['build', configName, testName]):
                    buildFail(False, testInfo)
                    continue
            runTest(testInfo)
        else:
            logTestSkip(testInfo.getFullName(), 'Unrecognized config ' + configName)

def runTest(testInfo):
    testPath = testInfo.getTestPath()
    try:
        if os.path.exists(testPath):
            p = subprocess.Popen(testPath)
            start = time.time()
            while p.returncode == None:
                p.poll()
                cur = time.time() - start
                if cur > 30:
                    p.kill()
                    logTestSkip(testInfo.getFullName(), 'Test timed out ( > 30 seconds)')
                    return
            if p.returncode != 0:
                logTestSkip(testInfo.getFullName(), 'Abnormal exit code ' + str(p.returncode) + '. Most likely caught exception.')
            else:
                processTestResult(testInfo)
        else:
            logTestSkip(testInfo.getFullName(), 'Unable to find ' + testPath)
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
    gLowLevelResultSummary.Name = 'Summary'
    html += lowLevelTestResultToHTML(gLowLevelResultSummary)
    for result in gLowLevelResultList:
        html += lowLevelTestResultToHTML(result)
    html += '</table>\n'
    return html

def systemTestResultToHTML(result):
    html = '<tr>'
    html += '<td>' + result.Name + '</td>\n'
    html += '<td>' + str(result.ErrorMargin * 100) + ' %</td>\n'
    compareResult = marginCompare(result.LoadTime, result.RefLoadTime, result.ErrorMargin)
    if(compareResult == 1):
        html += '<td bgcolor="red"><font color="white">' 
    elif(compareResult == -1):
        html += '<td bgcolor="green"><font color="white">' 
    else:
        html += '<td><font>' 
    html += str(result.LoadTime) + '</font></td>\n'
    html += '<td>' + str(result.RefLoadTime) + '</td>\n'

    compareResult = marginCompare(result.AvgFrameTime, result.RefAvgFrameTime, result.ErrorMargin)
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
    html += '<th colspan=\'6\'>System Test Results</th>\n'
    html += '</tr>\n'

    html += '<th>Test</th>\n'
    html += '<th>Error Margin</th>\n'
    html += '<th>LoadTime</th>\n'
    html += '<th>Ref LoadTime</th>\n'
    html += '<th>Avg FrameTime</th>\n'
    html += '<th>Ref FrameTime</th>\n'
    for result in gSystemResultList:
        html += systemTestResultToHTML(result)
    html += '</table>\n'
    return html

def getImageCompareResultsTable():
    max = 0
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
        html += '<tr>\n'
        html += '<td>' + result.Name + '</td>\n'
        for compare in result.CompareResults:
            if float(compare) > 0.1:
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

def outputHTML(openSummary):
    html = getLowLevelTestResultsTable()
    html += '<br><br>'
    html += getSystemTestResultsTable()
    html += '<br><br>'
    html += getImageCompareResultsTable()
    html += '<br><br>'
    html += getSkipsTable()
    resultSummaryName = gResultsDir + '\\TestSummary.html'
    outfile = open(resultSummaryName, 'w')
    outfile.write(html)
    outfile.close()
    if(openSummary):
        os.system("start " + resultSummaryName)

def cleanScreenShots(ssDir):
    if not os.path.isdir(ssDir):
        print 'Error. unable to clean screenshots, could not find ' + ssDir
        return
    screenshots = [s for s in os.listdir(ssDir) if s.endswith('.png')]
    for s in screenshots:
        os.remove(ssDir + '\\' + s)

def updateRepo():
    subprocess.call(['git', 'pull', 'origin', 'TestingFramework'])
    subprocess.call(['git', 'checkout', 'TestingFramework'])

def sendEmail():
    #try to find result dir from yesterday
    yesterdayStr = (date.today() - timedelta(days=1)).strftime("%m-%d-%y")
    slashIndex = gResultsDir.find('\\')
    yesterdayDir = gResultsDir[:slashIndex + 1] + yesterdayStr
    yesterdayFile = yesterdayDir + '\\TestSummary.html'
    todayFile = gResultsDir + '\\TestSummary.html'
    if os.path.isdir(yesterdayDir) and os.path.isfile(yesterdayFile):
        if filecmp.cmp(yesterdayFile, todayFile):
            subject = '[Unchanged] '
        else:
            subject = '[Different] '
    else:
        subject = '[No Reference Summary] '
    todayStr = (date.today()).strftime("%m-%d-%y")
    subject += 'Falcor Automated Testing for ' + todayStr
    body = 'Attached is the testing summary for ' + todayStr
    sender = 'clavelle@nvidia.com'
    recipients = str(open(gEmailRecipientFile, 'r').read());
    subprocess.call(['blat.exe', '-install', 'mail.nvidia.com', sender])
    subprocess.call(['blat.exe', '-to', recipients, '-subject', subject, '-body', body, '-attach', todayFile])

def main():
    global gResultsDir
    global gGenRefResults
    parser = argparse.ArgumentParser()
    parser.add_argument('-nb', '--nobuild', action='store_true', help='run without rebuilding Falcor and test apps')
    parser.add_argument('-np', '--nopull', action='store_true', help='run without pulling TestingFramework')
    parser.add_argument('-ne', '--noemail', action='store_true', help='run without emailing the result summary')
    parser.add_argument('-ss', '--showsummary', action='store_true', help='opens testing summary upon completion')
    parser.add_argument('-ctr', '--cleantestresults', action='store_true', help='deletes test results dir if exists')
    parser.add_argument('-gr', '--generatereference', action='store_true', help='generates reference testing logs and images')
    args = parser.parse_args()

    if args.generatereference:
        gGenRefResults = True
        if os.path.isdir(gReferenceDir):
            shutil.rmtree(gReferenceDir, ignore_errors=True)
        else:
            os.makedirs(gReferenceDir)

    if not args.nopull:
        updateRepo()

    cleanScreenShots(gDebugDir)
    cleanScreenShots(gReleaseDir)

    #make outer dir if need to
    makeDirIfDoesntExist(gResultsDir)
    #make inner dir for this results
    dateStr = date.today().strftime("%m-%d-%y")
    gResultsDir += '\\' + dateStr

    if args.cleantestresults:
        shutil.rmtree(gResultsDir, ignore_errors=True)
        try:
            os.makedirs(gResultsDir)
        except PermissionError, info:
            print 'Fatal Error, Failed to create test result folder. (just try again). Exception: ', info
    else:
        makeDirIfDoesntExist(gResultsDir)
 
    if not args.nobuild:
        callBatchFile(['clean', 'debugd3d12'])
        callBatchFile(['clean', 'released3d12'])
        #returns 1 on fail
        if callBatchFile(['build', 'debugd3d12', 'Falcor']):
            testInfo = TestInfo('Falcor', 'debugd3d12', .05)
            buildFail(True, testInfo)
        if callBatchFile(['build', 'released3d12', 'Falcor']):
            testInfo = TestInfo('Falcor', 'released3d12', .05)
            buildFail(True, testInfo)
        if callBatchFile(['build', 'debugd3d12', 'FalcorTest']):
            testInfo = TestInfo('FalcorTest', 'debugd3d12', .05)
            buildFail(True, testInfo)
        if callBatchFile(['build', 'released3d12', 'FalcorTest']):
            testInfo = TestInfo('FalcorTest', 'released3d12', .05)
            buildFail(True, 'FalcorTest', 'released3d12')
        #TODO, build other falcor configs, or better, only build configs listed in the test file

    readTestList(not args.nobuild)
    outputHTML(args.showsummary)

    if not args.noemail:
        sendEmail()
if __name__ == '__main__':
    main()