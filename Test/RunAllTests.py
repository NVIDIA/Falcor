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
import ntpath
from datetime import date, timedelta

#relevant paths
gBuildBatchFile = 'BuildFalcorTest.bat'
gTestListFile = 'TestList.txt'
gResultsDirBase = 'TestResults'
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

class TestSolution(object):
    def __init__(self, name):
        self.name = name
        self.systemResultList = []
        self.lowLevelResultList = []
        self.skippedList = []
        self.errorList = []
        dateStr = date.today().strftime("%m-%d-%y")
        self.resultsDir = gResultsDirBase + '\\' + dateStr
        makeDirIfDoesntExist(self.resultsDir)
        self.configDict = {}

class TestInfo(object):
    def __init__(self, name, configName, slnInfo):
        self.Name = name
        self.ConfigName = configName
        self.LoadErrorMargin = gDefaultLoadTimeMargin
        self.FrameErrorMargin = gDefaultFrameTimeMargin
        self.Index = 0
        self.slnInfo = slnInfo

    def determineIndex(self, generateReference,):
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

    def getResultsFile(self):
        return self.Name + '_TestingLog_' + str(self.Index) + '.xml'
    def getResultsDir(self):
        return self.slnInfo.resultsDir + '\\' + self.ConfigName
    def getReferenceDir(self):
        return gReferenceDir + '\\' + self.ConfigName
    def getReferenceFile(self):
        return self.getReferenceDir() + '\\' + self.getResultsFile()
    def getFullName(self):
        return self.Name + '_' + self.ConfigName + '_' + str(self.Index)
    def getTestDir(self):
        try:
            return self.slnInfo.configDict[self.ConfigName]
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

# -1 smaller, 0 same, 1 larger
def marginCompare(result, reference, margin):
    delta = result - reference
    if abs(delta) < reference * margin:
        return 0
    elif delta > 0:
        return 1
    else:
        return -1

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

def compareImages(resultObj, testInfo, numScreenshots, slnInfo):
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
            slnInfo.errorList.append(('For test ' + testInfo.getFullName() + 
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
            slnInfo.errorList.append(('For test ' + testInfo.getFullName() + ', screenshot ' +
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

def buildFail(slnName, configName, slnInfo):
    makeDirIfDoesntExist(slnInfo.resultsDir)
    buildLog = 'Solution_BuildFailLog.txt'
    overwriteMove(buildLog, slnInfo.resultsDir)
    errorMsg = 'failed to build one or more projects with config ' + configName + ' of solution ' + slnName  + ". Build log written to " + buildLog
    print 'Error: ' + errorMsg
    slnInfo.errorList.append(errorMsg)

def addCrash(test, slnInfo):
    slnInfo.skippedList.append(testName, 'Unhandled Crash')

def compareLowLevelReference(testInfo, testResult, slnInfo):
    if not os.path.isfile(testInfo.getReferenceFile()):
        slnInfo.errorList.append(('For test ' + testInfo.getFullName() + 
        ', could not find reference file ' + testInfo.getReferenceFile()))
        return
    refTag = getXMLTag(testInfo.getReferenceFile(), 'Summary')
    if refTag == None: 
        slnInfo.errorList.append(('For test ' + testInfo.getFullName() + 
        ', could not find reference file ' + testInfo.getReferenceFile()))
        return
    refTotal = int(refTag[0].attributes['TotalTests'].value)
    if testResult.Total != refTotal:
        slnInfo.errorList.append((testInfo.getFullName() + ': Number of tests is ' +
        str(testResult.Total) + ' which does not match ' + str(refTotal) + ' in reference'))
    refPassed = int(refTag[0].attributes['PassedTests'].value)
    if testResult.Passed != refPassed:
        slnInfo.errorList.append((testInfo.getFullName() + ': Number of passed tests is ' + 
        str(testResult.Passed) + ' which does not match ' + str(refPassed) + ' in reference'))
    refFailed = int(refTag[0].attributes['FailedTests'].value)
    if testResult.Failed != refFailed:
        slnInfo.errorList.append((testInfo.getFullName() + ': Number of failed tests is ' + 
        str(testResult.Failed) + ' which does not match ' + str(refFailed) + ' in reference'))
    refCrashed = int(refTag[0].attributes['CrashedTests'].value)
    if testResult.Crashed != refCrashed:
        slnInfo.errorList.append((testInfo.getFullName() + ': Number of crashed tests is ' +  
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

def addToLowLevelSummary(slnInfo, result):
    if not slnInfo.lowLevelResultList:
        resultSummary = LowLevelResult()
        resultSummary.Name = 'Summary'
        slnInfo.lowLevelResultList.append(resultSummary)
    slnInfo.lowLevelResultList[0].add(result)
    slnInfo.lowLevelResultList.append(result)

def processLowLevelTest(xmlElement, testInfo, slnInfo):
    newResult = LowLevelResult()
    newResult.Name = testInfo.getFullName()
    newResult.Total = int(xmlElement[0].attributes['TotalTests'].value)
    newResult.Passed = int(xmlElement[0].attributes['PassedTests'].value)
    newResult.Failed = int(xmlElement[0].attributes['FailedTests'].value)
    newResult.Crashed = int(xmlElement[0].attributes['CrashedTests'].value)
    addToLowLevelSummary(slnInfo, newResult)
    slnInfo.lowLevelResultList.append(newResult)
    #result at index 0 is summary
    makeDirIfDoesntExist(testInfo.getResultsDir())
    overwriteMove(testInfo.getResultsFile(), testInfo.getResultsDir())
    compareLowLevelReference(testInfo, newResult, slnInfo)

def processSystemTest(xmlElement, testInfo, slnInfo):
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
        slnInfo.skippedList.append((testInfo.getFullName(), 'Could not find reference file ' + referenceFile + ' for comparison'))
        return 
    refResults = getXMLTag(referenceFile, 'Summary')
    if refResults == None:
        slnInfo.skippedList.append((testInfo.getFullName(), 'Error getting xml data from reference file ' + referenceFile))
        return
    newSysResult.RefLoadTime = float(refResults[0].attributes['LoadTime'].value)
    newSysResult.RefAvgFrameTime = float(refResults[0].attributes['FrameTime'].value)
    #check avg fps
    if newSysResult.AvgFrameTime != 0 and newSysResult.RefAvgFrameTime != 0:
        if marginCompare(newSysResult.AvgFrameTime, newSysResult.RefAvgFrameTime, newSysResult.FrameErrorMargin) == 1:
            slnInfo.errorList.append((testInfo.getFullName() + ': average frame time ' + 
            str(newSysResult.AvgFrameTime) + ' is larger than reference ' + str(newSysResult.RefAvgFrameTime) + 
            ' considering error margin ' + str(newSysResult.FrameErrorMargin * 100) + '%'))
    #check load time
    if newSysResult.LoadTime != 0 and newSysResult.RefLoadTime != 0:
        if newSysResult.LoadTime > (newSysResult.RefLoadTime + newSysResult.LoadErrorMargin):
            slnInfo.errorList.append(testInfo.getFullName() + ': load time' + (str(newSysResult.LoadTime) +
            ' is larger than reference ' + str(newSysResult.RefLoadTime) + ' considering error margin ' + 
            str(newSysResult.LoadErrorMargin) + ' seconds'))
    compareImages(newSysResult, testInfo, numScreenshots, slnInfo)
    slnInfo.systemResultList.append(newSysResult)
    makeDirIfDoesntExist(testInfo.getResultsDir())
    overwriteMove(resultFile, testInfo.getResultsDir())

def readTestList(generateReference, buildTests, pullBranch):
    file = open(gTestListFile)
    contents = file.read()

    slnInfos = []

    slnDataStartIndex = contents.find('[')
    while slnDataStartIndex != -1:
        #get all data about testing this solution
        slnDataEndIndex = contents.find(']')
        solutionData = contents[slnDataStartIndex + 1 : slnDataEndIndex]
        slnEndIndex = solutionData.find(' ')
        slnName = cleanupString(solutionData[:slnEndIndex])

        #make sln name dir within date dir
        slnBaseName, extension = os.path.splitext(slnName)
        slnBaseName = ntpath.basename(slnBaseName)
        slnInfo = TestSolution(slnBaseName)
        slnInfos.append(slnInfo)
        if pullBranch:
            slnInfo.resultsDir += '\\' + slnBaseName + '_' + pullBranch
        else:
            slnInfo.resultsDir += '\\' + slnBaseName
        if os.path.isdir(slnInfo.resultsDir):
            cleanDir(slnInfo.resultsDir, None, None)
        else:
            os.makedirs(slnInfo.resultsDir)

        #parse solutiondata
        slnConfigStartIndex = solutionData.find('{')
        slnConfigEndIndex = solutionData.find('}')
        configData = solutionData[slnConfigStartIndex + 1 : slnConfigEndIndex]
        configDataList = configData.split(' ')
        for i in xrange(0, len(configDataList), 2):
            exeDir = configDataList[i + 1]
            configName = configDataList[i].lower()
            slnInfo.configDict[configName] = exeDir
            if buildTests:
                callBatchFile(['clean', slnName, configName])
                #delete bin dir
                if os.path.isdir(exeDir):
                    shutil.rmtree(exeDir)
                #returns 1 on fail
                if callBatchFile(['rebuild', slnName, configName]):
                    buildFail(slnName, configName, slnInfo)
            else:
                cleanDir(exeDir, None, '.png')

        #move buffer to beginning of args
        solutionData = solutionData[slnConfigEndIndex + 1 :]

        #parse arg data
        argStartIndex = solutionData.find('{')
        while(argStartIndex != -1):
            testName = cleanupString(solutionData[:argStartIndex])
            argEndIndex = solutionData.find('}')
            args = cleanupString(solutionData[argStartIndex + 1 : argEndIndex])
            argsList = args.split(',')
            solutionData = solutionData[argEndIndex + 1 :]
            configStartIndex = solutionData.find('{')
            configEndIndex = solutionData.find('}')
            configList = cleanupString(solutionData[configStartIndex + 1 : configEndIndex]).split(' ')
            #run test for each config and each set of args
            for config in configList:
                print 'Running ' + testName + ' in config ' + config 
                testInfo = TestInfo(testName, config, slnInfo)
                if generateReference:
                    cleanDir(testInfo.getReferenceDir(), testName, '.png')
                    cleanDir(testInfo.getReferenceDir(), testName, '.xml')
                for argSet in argsList:
                    testInfo = TestInfo(testName, config, slnInfo)
                    runTest(testInfo, cleanupString(argSet), generateReference, slnInfo)

            #goto next set
            solutionData = solutionData[configEndIndex + 1 :]
            argStartIndex = solutionData.find('{')
        #goto next solution set
        contents = contents[slnDataEndIndex + 1 :]
        slnDataStartIndex = contents.find('[')
    return slnInfos

def runTest(testInfo, cmdLine, generateReference, slnInfo):
    testPath = testInfo.getTestPath()
    if not os.path.exists(testPath):
        slnInfo.skippedList.append((testInfo.getFullName(), 'Unable to find ' + testPath))
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
                slnInfo.skippedList.append((testInfo.getFullName(), ('Test timed out ( > ' + 
                    str(gDefaultHangTimeDuration) + ' seconds)')))
                return
        #ensure results file exists
        if not os.path.isfile(testInfo.getResultsFile()):
            slnInfo.skippedList.append((testInfo.getFullName(), 'Failed to open test result file ' + testInfo.getResultsFile()))
            return
        #check for name conflicts
        testInfo.determineIndex(generateReference)
        #get xml from results file
        summary = getXMLTag(testInfo.getResultsFile(), 'Summary')
        if summary == None:
            slnInfo.skippedList.append((testInfo.getFullName(), 'Error getting xml data from ' + testInfo.getResultsFile()))
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
            processSystemTest(summary, testInfo, slnInfo)
        #gen low level ref
        elif generateReference:
            makeDirIfDoesntExist(testInfo.getReferenceDir())
            overwriteMove(testInfo.getResultsFile(), testInfo.getReferenceDir())
        #process low level
        else:
            processLowLevelTest(summary, testInfo, slnInfo)
    except subprocess.CalledProcessError:
        addCrash(testInfo.getFullName(), slnInfo)

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

def getLowLevelTestResultsTable(slnInfo):
    if slnInfo.lowLevelResultList:
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
        for result in slnInfo.lowLevelResultList:
            html += lowLevelTestResultToHTML(result)
        html += '</table>\n'
        return html
    else:
        return ''

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

def getSystemTestResultsTable(slnInfo):
    if slnInfo.systemResultList:
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
        for result in slnInfo.systemResultList:
            html += systemTestResultToHTML(result)
        html += '</table>\n'
        return html
    else:
        return ''

def getImageCompareResultsTable(slnInfo):
    if slnInfo.systemResultList:
        max = 0
        #table needs max num of screenshots plus one columns 
        for result in slnInfo.systemResultList:
            if len(result.CompareResults) > max:
                max = len(result.CompareResults)
        html = '<table style="width:100%" border="1">\n'
        html += '<tr>\n'
        html += '<th colspan=\'' + str(max + 1) + '\'>Image Compare Tests</th>\n'
        html += '</tr>\n'
        html += '<th>Test</th>\n'
        for i in range (0, max):
            html += '<th>SS' + str(i) + '</th>\n'
        for result in slnInfo.systemResultList:
            if len(result.CompareResults) > 0:
                html += '<tr>\n'
                html += '<td>' + result.Name + '</td>\n'
                for compare in result.CompareResults:
                    if float(compare) > gDefaultImageCompareMargin or float(compare) < 0:
                        html += '<td bgcolor="red"><font color="white">' + str(compare) + '</font></td>\n'
                    else:
                        html += '<td>' + str(compare) + '</td>\n'
                html += '</tr>\n'
        html += '</table>\n'
        return html
    else:
        return ''

def skipToHTML(name, reason):
    html = '<tr>\n'
    html += '<td bgcolor="red"><font color="white">' + name + '</font></td>\n'
    html += '<td>' + reason + '</td>\n'
    html += '</tr>\n'
    return html

def getSkipsTable(slnInfo):
    if slnInfo.skippedList:
        html = '<table style="width:100%" border="1">\n'
        html += '<tr>\n'
        html += '<th colspan=\'2\'>Skipped Tests</th>'
        html += '</tr>'
        html += '<th>Test</th>\n'
        html += '<th>Reason for Skip</th>\n'
        for name, reason in slnInfo.skippedList:
            html += skipToHTML(name, reason)
        html += '</table>'
        return html
    else:
        return ''

def outputHTML(openSummary, slnInfo, pullBranch):
    html = getLowLevelTestResultsTable(slnInfo)
    html += '<br><br>'
    html += getSystemTestResultsTable(slnInfo)
    html += '<br><br>'
    html += getImageCompareResultsTable(slnInfo)
    html += '<br><br>'
    html += getSkipsTable(slnInfo)
    if pullBranch:
        resultSummaryName = slnInfo.resultsDir + '\\' + slnInfo.name + '_' + pullBranch + '_TestSummary.html'
    else:
        resultSummaryName = slnInfo.resultsDir + '\\' + slnInfo.name + '_TestSummary.html'
    outfile = open(resultSummaryName, 'w')
    outfile.write(html)
    outfile.close()
    if(openSummary):
        os.system("start " + resultSummaryName)

def cleanDir(cleanedDir, prefix, suffix):
    if os.path.isdir(cleanedDir):
        if prefix and suffix:
            deadFiles = [f for f in os.listdir(cleanedDir) if f.endswith(suffix) and f.startswith(prefix)]
        elif prefix:
            deadFiles = [f for f in os.listdir(cleanedDir) if f.startswith(prefix)]
        elif suffix:
            deadFiles = [f for f in os.listdir(cleanedDir) if f.endswith(suffix)]
        else:
            deadFiles = [f for f in os.listdir(cleanedDir)]
        for f in deadFiles:
            filepath = cleanedDir + '\\' + f
            if os.path.isdir(filepath):
                cleanDir(filepath, prefix, suffix)
            else:
                os.remove(filepath)

def main(build, showSummary, generateReference, referenceDir, testList, pullBranch):
    global gReferenceDir
    global gTestListFile

    refDir = cleanupString(referenceDir)
    if os.path.isdir(refDir):
        gReferenceDir = refDir
    elif not generateReference:
        print 'Fatal Error, Failed to find reference dir: ' + referenceDir
        sys.exit(1)

    if generateReference:
        if not os.path.isdir(gReferenceDir):
            try:
                os.makedirs(gReferenceDir)
            except:
                print 'Fatal Error, Failed to create reference dir.'
                sys.exit(1)

    if not os.path.exists(testList):
        print 'Error, Failed to find test list ' + testList 
    else:
        gTestListFile = testList

    #make outer dir if need to
    makeDirIfDoesntExist(gResultsDirBase)

    slnInfos = readTestList(generateReference, build, pullBranch)
    if not generateReference:
        for sln in slnInfos:
            outputHTML(showSummary, sln, pullBranch)

    testingResults = []
    for sln in slnInfos:
        slnResultPath =  os.getcwd() + '\\' + sln.resultsDir + '\\' + sln.name
        if pullBranch:
            slnResultPath += '_' + pullBranch
        resultSummary = slnResultPath + '_TestSummary.html'
        errorSummary = slnResultPath + '_ErrorSummary.txt'

        if(len(sln.skippedList) > 0 or len(sln.errorList) > 0):
            errorStr = ''
            for name, skip in sln.skippedList:
                errorStr += name + ': ' + skip + '\n'
            for reason in sln.errorList:
                errorStr += reason + '\n' 
            errorFile = open(errorSummary, 'w')
            errorFile.write(errorStr)
            errorFile.close()
            testingResults.append([resultSummary, False])
        else:
            testingResults.append([resultSummary, True])

    #check len for pass fail, len 2 includes error file, which is a fail  
    return  testingResults

def cleanupString(string):
    string = string.replace('\t', '')
    return string.replace('\n', '').strip()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-nb', '--nobuild', action='store_true', help='run without rebuilding Falcor and test apps')
    parser.add_argument('-ss', '--showsummary', action='store_true', help='opens testing summary upon completion')
    parser.add_argument('-gr', '--generatereference', action='store_true', help='generates reference testing logs and images')
    parser.add_argument('-ref', '--referencedir', action='store', help='Allows user to specify an existing reference dir')
    parser.add_argument('-tests', '--testlist', action='store', help='Allows user to specify the test list file')
    args = parser.parse_args()

    if args.referencedir:
        refDir = args.referencedir
    else:
        refDir = gReferenceDir

    if args.testlist:
        testList = args.testlist
    else:
        testList = gTestListFile

    #final arg is pull branch, just to name subdirectories in the same repo folder so results dont overwrite 
    main(not args.nobuild, args.showsummary, args.generatereference, refDir, testList, '')
