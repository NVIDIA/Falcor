import subprocess
import shutil
import time
import os
from xml.dom import minidom
from xml.parsers.expat import ExpatError
import argparse
import sys
import ntpath
from datetime import date
#custom written modules
import OutputTestingHtml as htmlWriter
import TestingUtil as testingUtil

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
#seconds
gDefaultHangTimeDuration = 1800

#stores data for all tests in a particular solution
class TestSolution(object):
    def __init__(self, name):
        self.name = name
        self.systemResultList = []
        self.lowLevelResultList = []
        self.skippedList = []
        self.errorList = []
        dateStr = date.today().strftime("%m-%d-%y")
        self.resultsDir = gResultsDirBase + '\\' + dateStr
        testingUtil.makeDirIfDoesntExist(self.resultsDir)
        self.configDict = {}

#test info class stores test data and has functions that require test data, mostly getting necessary file paths
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
            testingUtil.overwriteMove(initialFilename, self.getResultsFile())

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

def renameScreenshots(testInfo, numScreenshots):
    for i in range (0, numScreenshots):
        os.rename(testInfo.getInitialTestScreenshot(i), testInfo.getRenamedTestScreenshot(i))

def compareImages(resultObj, testInfo, numScreenshots, slnInfo):
    renameScreenshots(testInfo, numScreenshots)
    for i in range(0, numScreenshots):
        testScreenshot = testInfo.getRenamedTestScreenshot(i)
        refScreenshot = testInfo.getReferenceScreenshot(i)
        outFile = testInfo.Name + '_' + str(testInfo.Index) + '_' + str(i) + '_Compare.png'
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
            testingUtil.makeDirIfDoesntExist(imagesDir)
            testingUtil.overwriteMove(testScreenshot, imagesDir)
            resultObj.CompareResults.append(-1)
            continue
        resultObj.CompareResults.append(resultVal)

        # Move images to results folder
        testingUtil.makeDirIfDoesntExist(imagesDir)
        testingUtil.overwriteMove(testScreenshot, imagesDir)
        testingUtil.overwriteMove(outFile, imagesDir)

        #if the images are sufficiently different, save them in test results
        if resultVal > testingUtil.gDefaultImageCompareMargin:
            slnInfo.errorList.append(('For test ' + testInfo.getFullName() + ', screenshot ' +
                testScreenshot + ' differs from ' + refScreenshot + ' by ' + result +
                ' average difference per pixel. (Exceeds threshold .01)'))

def addSystemTestReferences(testInfo, numScreenshots):
    renameScreenshots(testInfo, numScreenshots)
    referenceDir = testInfo.getReferenceDir()
    testingUtil.makeDirIfDoesntExist(referenceDir)
    testingUtil.overwriteMove(testInfo.getResultsFile(), referenceDir)
    for i in range(0, numScreenshots):
        testingUtil.overwriteMove(testInfo.getRenamedTestScreenshot(i), referenceDir)

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
        print 'Incorrect batch file call, found ' + str(numArgs) + ' in arg list :' + batchArgs.tostring()
        return 1

def buildFail(slnName, configName, slnInfo):
    testingUtil.makeDirIfDoesntExist(slnInfo.resultsDir)
    buildLog = 'Solution_BuildFailLog.txt'
    testingUtil.overwriteMove(buildLog, slnInfo.resultsDir)
    errorMsg = 'failed to build one or more projects with config ' + configName + ' of solution ' + slnName  + ". Build log written to " + buildLog
    print 'Error: ' + errorMsg
    slnInfo.errorList.append(errorMsg)

def addCrash(test, slnInfo):
    slnInfo.skippedList.append(test, 'Unhandled Crash')

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
    if newResult.Failed > 0:
        slnInfo.errorList.append(testInfo.getFullName() + ' had ' + str(newResult.Failed) + ' failed tests')
    newResult.Crashed = int(xmlElement[0].attributes['CrashedTests'].value)
    addToLowLevelSummary(slnInfo, newResult)
    slnInfo.lowLevelResultList.append(newResult)
    #result at index 0 is summary
    testingUtil.makeDirIfDoesntExist(testInfo.getResultsDir())
    testingUtil.overwriteMove(testInfo.getResultsFile(), testInfo.getResultsDir())

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
    if not refResults:
        slnInfo.skippedList.append((testInfo.getFullName(), 'Error getting xml data from reference file ' + referenceFile))
        return
    newSysResult.RefLoadTime = float(refResults[0].attributes['LoadTime'].value)
    newSysResult.RefAvgFrameTime = float(refResults[0].attributes['FrameTime'].value)
    #check avg fps
    if newSysResult.AvgFrameTime != 0 and newSysResult.RefAvgFrameTime != 0:
        if testingUtil.marginCompare(newSysResult.AvgFrameTime, newSysResult.RefAvgFrameTime, newSysResult.FrameErrorMargin) == 1:
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
    testingUtil.makeDirIfDoesntExist(testInfo.getResultsDir())
    testingUtil.overwriteMove(resultFile, testInfo.getResultsDir())

def readTestList(generateReference, buildTests, pullBranch):
    testFile = open(gTestListFile)
    contents = testFile.read()

    slnInfos = []

    slnDataStartIndex = contents.find('[')
    while slnDataStartIndex != -1:
        #get all data about testing this solution
        slnDataEndIndex = contents.find(']')
        solutionData = contents[slnDataStartIndex + 1 : slnDataEndIndex]
        slnEndIndex = solutionData.find(' ')
        slnName = testingUtil.cleanupString(solutionData[:slnEndIndex])

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
            testingUtil.cleanDir(slnInfo.resultsDir, None, None)
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
                testingUtil.cleanDir(exeDir, None, '.png')

        #move buffer to beginning of args
        solutionData = solutionData[slnConfigEndIndex + 1 :]

        #parse arg data
        argStartIndex = solutionData.find('{')
        while argStartIndex != -1:
            testName = testingUtil.cleanupString(solutionData[:argStartIndex])
            argEndIndex = solutionData.find('}')
            argString = testingUtil.cleanupString(solutionData[argStartIndex + 1 : argEndIndex])
            argsList = argString.split(',')
            solutionData = solutionData[argEndIndex + 1 :]
            configStartIndex = solutionData.find('{')
            configEndIndex = solutionData.find('}')
            configList = testingUtil.cleanupString(solutionData[configStartIndex + 1 : configEndIndex]).split(' ')
            #run test for each config and each set of args
            for config in configList:
                print 'Running ' + testName + ' in config ' + config
                testInfo = TestInfo(testName, config, slnInfo)
                if generateReference:
                    testingUtil.cleanDir(testInfo.getReferenceDir(), testName, '.png')
                    testingUtil.cleanDir(testInfo.getReferenceDir(), testName, '.xml')
                for argSet in argsList:
                    testInfo = TestInfo(testName, config, slnInfo)
                    runTest(testInfo, testingUtil.cleanupString(argSet), generateReference, slnInfo)

            #goto next set
            solutionData = solutionData[configEndIndex + 1 :]
            argStartIndex = solutionData.find('{')
        #goto next solution set
        contents = contents[slnDataEndIndex + 1 :]
        slnDataStartIndex = contents.find('[')
    return slnInfos

def runTest(testInfo, cmdLine, generateReference, slnInfo):
    #dont generate low level reference
    if generateReference and not cmdLine:
        return

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
        if not summary:
            slnInfo.skippedList.append((testInfo.getFullName(), 'Error getting xml data from ' + testInfo.getResultsFile()))
            if generateReference:
                testingUtil.makeDirIfDoesntExist(testInfo.getReferenceDir())
                testingUtil.overwriteMove(testInfo.getResultsFile(), testInfo.getReferenceDir())
            else:
                testingUtil.makeDirIfDoesntExist(testInfo.getResultsDir())
                testingUtil.overwriteMove(testInfo.getResultsFile(), testInfo.getResultsDir())
            return
        #gen system ref
        if cmdLine and generateReference:
            numScreenshots = int(summary[0].attributes['NumScreenshots'].value)
            addSystemTestReferences(testInfo, numScreenshots)
        #process system
        elif cmdLine:
            processSystemTest(summary, testInfo, slnInfo)

        #process low level
        else:
            processLowLevelTest(summary, testInfo, slnInfo)
    except subprocess.CalledProcessError:
        addCrash(testInfo.getFullName(), slnInfo)

def main(build, showSummary, generateReference, referenceDir, testList, pullBranch):
    global gReferenceDir
    global gTestListFile

    reference = testingUtil.cleanupString(referenceDir)
    if os.path.isdir(reference):
        gReferenceDir = reference
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
        print 'Fatal Error, Failed to find test list ' + testList
        sys.exit(1)
    else:
        gTestListFile = testList

    #make outer dir if need to
    testingUtil.makeDirIfDoesntExist(gResultsDirBase)

    #Read test list, run tests, and return results in a list of slnInfo classes
    slnInfos = readTestList(generateReference, build, pullBranch)
    if not generateReference:
        for sln in slnInfos:
            htmlWriter.outputHTML(showSummary, sln, pullBranch)

    #parse slninfo classes to fill testing result list to return to calling script
    testingResults = []
    for sln in slnInfos:
        slnResultPath =  os.getcwd() + '\\' + sln.resultsDir + '\\' + sln.name
        if pullBranch:
            slnResultPath += '_' + pullBranch
        resultSummary = slnResultPath + '_TestSummary.html'
        errorSummary = slnResultPath + '_ErrorSummary.txt'

        if len(sln.skippedList) > 0 or len(sln.errorList) > 0:
            errorStr = ''
            for name, skip in sln.skippedList:
                errorStr += name + ': ' + skip + '\n'
            for reason in sln.errorList:
                errorStr += reason + '\n'
            errorFile = open(errorSummary, 'w')
            errorFile.write(errorStr)
            errorFile.close()
            testingResults.append([sln.name, resultSummary, False])
        else:
            testingResults.append([sln.name, resultSummary, True])

    #return results back to calling script
    return testingResults

#Main is separated from this so other scripts (CallTestingScript.py) can import this script as a module and
#call main as a function. All this __main__ does is parse command line arguments and pass them into the main function
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
        testListFile = args.testlist
    else:
        testListFile = gTestListFile

    #final arg is pull branch, just to name subdirectories in the same repo folder so results dont overwrite
    main(not args.nobuild, args.showsummary, args.generatereference, refDir, testListFile, '')
