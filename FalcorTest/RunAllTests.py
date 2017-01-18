import subprocess
from subprocess import PIPE
import shutil
import time
import os
from xml.dom import minidom
from xml.parsers.expat import ExpatError
import argparse
import sys

testListFile = 'C:\\Users\\clavelle\\Desktop\\FalcorGitHub\\FalcorTest\\TestList.txt'
buildBatchFile = 'BuildFalcorTest.bat'
debugFolder = 'C:\\Users\\clavelle\\Desktop\\FalcorGitHub\\FalcorTest\\Bin\\x64\\Debug\\'
releaseFolder = 'C:\\Users\\clavelle\\Desktop\\FalcorGitHub\\FalcorTest\\Bin\\x64\\Release\\'
resultsFolder = 'TestResults'
referenceFolder = 'SystemTestReferenceResults'

class SystemResult(object):
    def __init__(self):
        self.Name = ''
        self.LoadTime = 0
        self.AvgFrameTime = 0
        self.RefLoadTime = 0
        self.RefAvgFrameTime = 0
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

systemResultList = []
resultList = []
skippedList = []
resultSummary = TestResults()

def overwriteMove(filename, newLocation):
    try:
        shutil.copy(filename, newLocation)
        os.remove(filename)
    except IOError, info:
        print 'Error moving ' + filename + ' to ' + newLocation + '. Exception: ', info
        return

def compareImages(resultObj, testName, configName, numScreenshots):
    for i in range(0, numScreenshots):
        if configName == 'debugd3d12' or configName == 'debugd3d11' or configName == 'debugGL':
            testFolder = debugFolder
        elif configName == 'released3d12' or configName == 'released3d11' or configName == 'releaseGL':
            testFolder = releaseFolder
        else:
            print 'This should never get here, config valid in processSysTest but not in compareImages'
            return
        refFolder = referenceFolder + '\\' + configName
        testScreenshot = testFolder + '\\' + testName + '.exe.' + str(i) + '.png'
        refScreenshot = refFolder + '\\' + testName + '.exe.' + str(i) + '.png'
        outFile = testName + str(i) + '_Compare.png'
        command = ['magick', 'compare', '-metric', 'MSE', '-compose', 'Src', '-highlight-color', 'White', 
        '-lowlight-color', 'Black', testScreenshot, refScreenshot, outFile]
        p = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
        result = p.communicate()[0]
        spaceIndex = result.find(' ')
        result = result[:spaceIndex]
        resultObj.CompareResults.append(result)
        #if the images are sufficently different, save them in test results
        if float(result) > 0.01:
            imagesFolder = resultsFolder + '\\' + configName + '\\Images'
            if not os.path.isdir(imagesFolder):
                os.makedirs(imagesFolder)
            overwriteMove(testScreenshot, imagesFolder)
            overwriteMove(outFile, imagesFolder)
        #else just delete them
        else:
            os.remove(testScreenshot)
            os.remove(outFile)

def logTestSkip(testName, reason):
    global skippedList
    skippedList.append((testName, reason))

#args should be action(build, rebuild, clean), config(debug, release), and optionally project
#if no project given, performs action on entire solution
def callBatchFile(batchArgs):
    numArgs = len(batchArgs)
    if numArgs == 2 or numArgs == 3:
        batchArgs.insert(0, buildBatchFile)
        try:
            return subprocess.call(batchArgs)
        except WindowsError, info:
            print 'Error calling batch file. Exception: ', info
            return 1
    else:
        print 'Incorrect batch file call, found ' + string(numArgs) + ' in arg list :' + batchArgs.tostring()
        return 1

def buildFail(fatal, testName, configName):
    configDir = resultsFolder + '\\' + configName
    if not os.path.isdir(configDir):
        os.makedirs(configDir)
    buildFailLog = testName + "_BuildFailLog.txt"
    overwriteMove(buildFailLog, configDir)
    if fatal:
        print 'Fatal error, failed to build ' + testName
        sys.exit(1)
    else:
        logTestSkip(testName, 'Build Failure')

#Test Base tries to catch any thrown exceptions, but it's possible that the
#program might crash in a different way
def addCrash(testName):
    logTestSkip(testName, 'Unhandled Crash')

def processLowLevelTest(xmlElement, nameAndConfig, resultFile, configDir):
    global resultList
    global resultSummary
    newResult = TestResults()
    newResult.Name = nameAndConfig
    newResult.Total = int(xmlElement[0].attributes['TotalTests'].value)
    newResult.Passed = int(xmlElement[0].attributes['PassedTests'].value)
    newResult.Failed = int(xmlElement[0].attributes['FailedTests'].value)
    newResult.Crashed = int(xmlElement[0].attributes['CrashedTests'].value)
    resultList.append(newResult)
    resultSummary.add(newResult)
    overwriteMove(resultFile, configDir)

def processSystemTest(xmlElement, testName, configName, resultFile, configDir):
    global systemResultList
    newSysResult = SystemResult()
    nameAndConfig = testName + '_' + configName
    newSysResult.Name = nameAndConfig
    newSysResult.LoadTime = float(xmlElement[0].attributes['LoadTime'].value)
    newSysResult.AvgFrameTime = float(xmlElement[0].attributes['FrameTime'].value)
    numScreenshots = int(xmlElement[0].attributes['NumScreenshots'].value)
    referenceFile = referenceFolder + '\\' + configName + '\\' + resultFile
    if os.path.isfile(referenceFile):
        try:
            referenceDoc = minidom.parse(referenceFile)
        except ExpatError:
            logTestSkip(nameAndConfig, resultFile + ' was found, but is not correct xml')
            newSysResult.RefLoadTime = -1
            newSysResult.RefAvgFrameTime = -1
            overwriteMove(resultFile, configDir)
            return

        refResults = referenceDoc.getElementsByTagName('SystemResults')
        if len(refResults) != 1:
            logTestSkip(nameAndConfig, referenceFile + ' was found and is XML, but is improperly formatted')
            newSysResult.RefLoadTime = -1
            newSysResult.RefAvgFrameTime = -1
        else:
            newSysResult.RefLoadTime = float(refResults[0].attributes['LoadTime'].value)
            newSysResult.RefAvgFrameTime = float(refResults[0].attributes['FrameTime'].value)
            compareImages(newSysResult, testName, configName, numScreenshots)
    else:
        print 'Could not find reference file ' + referenceFile + ' for comparison'
        newSysResult.RefLoadTime = -1
        newSysResult.RefAvgFrameTime = -1
    systemResultList.append(newSysResult)
    overwriteMove(resultFile, configDir)

def processTestResult(testName, configName):
    resultFile = testName + "_TestingLog.xml" 
    if os.path.isfile(resultFile):
        #create config dir if it doesn't exist
        configDir = resultsFolder + '\\' + configName
        if not os.path.isdir(configDir):
            os.makedirs(configDir)

        nameAndConfig = testName + '_' + configName 
        try:
            xmlDoc = minidom.parse(resultFile)
        except ExpatError:
            logTestSkip(nameAndConfig, resultFile + ' was found, but is not correct xml')
            overwriteMove(resultFile, configDir)
            return

        lowLevelResults = xmlDoc.getElementsByTagName('Summary')
        sysResults  = xmlDoc.getElementsByTagName('SystemResults')
        if len(lowLevelResults) == 1:
            processLowLevelTest(lowLevelResults, nameAndConfig, resultFile, configDir)
        elif len(sysResults) ==  1: 
            processSystemTest(sysResults, testName, configName, resultFile, configDir)
        else:
            logTestSkip(nameAndConfig, resultFile + ' was found and is XML, but is improperly formatted')
            overwriteMove(resultFile, configDir)
    else:
        logTestSkip(testName, 'Unable to find result file ' + resultFile)

def readTestList(buildTests, cleanTests):
    testList = open(testListFile)
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
        configName = configName.lower()
        if (configName == 'debugd3d12' or configName == 'released3d12' or 
            configName == 'debugd3d11' or configName == 'released3d11' or
            configName == 'debugGL' or configName == 'releaseGL'):
            if buildTests:
                if cleanTests:
                    callBatchFile(['clean', configName, testName])
                #returns 1 on fail
                if callBatchFile(['build', configName, testName]):
                    buildFail(False, testName, configName)
                    continue
            runTest(testName, configName)
        else:
            logTestSkip(testName + "_" + configName, 'Unrecognized config ' + configName)

def runTest(testName, configName):
    if configName == 'debug' or configName == 'debugd3d12':
        testPath = debugFolder + testName + ".exe"
    elif configName == 'release' or configName == 'released3d12':
        testPath = releaseFolder + testName + ".exe"

    try:
        if os.path.exists(testPath):
            subprocess.check_call([testPath])
            processTestResult(testName, configName)
        else:
            logTestSkip(testName + '_' + configName, 'Unable to find ' + testPath)
    except subprocess.CalledProcessError:
        addCrash(testName)

def testResultToHTML(result):
    html = '<tr><font>'
    if int(result.Crashed) > 0:
        html = '<tr bgcolor="yellow"><font>'
    elif int(result.Failed) > 0:
        html = '<tr bgcolor="red"><font color="white">'
    html += '<td>' + result.Name + '</td>\n'
    html += '<td>' + str(result.Total) + '</td>\n'
    html += '<td>' + str(result.Passed) + '</td>\n'
    html += '<td>' + str(result.Failed) + '</td>\n'
    html += '<td>' + str(result.Crashed) + '</td>\n'
    html += '</font></tr>\n'
    return html

def getTestResultsTable():
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
    resultSummary.Name = 'Summary'
    html += testResultToHTML(resultSummary)
    for result in resultList:
        html += testResultToHTML(result)
    html += '</table>\n'
    return html

def systemTestResultToHTML(result):
    html = '<tr>'
    html += '<td>' + result.Name + '</td>\n'
    if(result.LoadTime > result.RefLoadTime):
        html += '<td bgcolor="red">' +'<font color="white">' 
        html += str(result.LoadTime) + '</font></td>\n'
    else:
        html += '<td bgcolor="green">' +'<font color="white">' 
        html += str(result.LoadTime) + '</font></td>\n'
    html += '<td>' + str(result.RefLoadTime) + '</td>\n'
    if(result.AvgFrameTime > result.RefAvgFrameTime):
        html += '<td bgcolor="red">' +'<font color="white">' 
        html += str(result.AvgFrameTime) + '</font></td>\n'
    else:
        html += '<td bgcolor="green">' +'<font color="white">' 
        html += str(result.AvgFrameTime) + '</font></td>\n'    
    html += '<td>' + str(result.RefAvgFrameTime) + '</td>\n'
    html += '</tr>\n'
    return html

def getSystemTestResultsTable():
    html = '<table style="width:100%" border="1">\n'
    html += '<tr>\n'
    html += '<th colspan=\'5\'>System Test Results</th>\n'
    html += '</tr>\n'
    html += '<th>Test</th>\n'
    html += '<th>LoadTime</th>\n'
    html += '<th>Ref LoadTime</th>\n'
    html += '<th>Avg FrameTime</th>\n'
    html += '<th>Ref FrameTime</th>\n'
    for result in systemResultList:
        html += systemTestResultToHTML(result)
    html += '</table>\n'
    return html

def getImageCompareResultsTable():
    max = 0
    for result in systemResultList:
        if len(result.CompareResults) > max:
            max = len(result.CompareResults)
    html = '<table style="width:100%" border="1">\n'
    html += '<tr>\n'
    html += '<th colspan=\'' + str(max + 1) + '\'>Image Compare Tests</th>\n'
    html += '</tr>\n'
    html += '<th>Test</th>\n'
    for i in range (0, max):
        html += '<th>SS' + str(i) + '</th>\n'
    for result in systemResultList:
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
    for name, reason in skippedList:
        html += skipToHTML(name, reason)
    html += '</table>'
    return html

def outputHTML(openSummary):
    html = getTestResultsTable()
    html += '<br><br>'
    html += getSystemTestResultsTable()
    html += '<br><br>'
    html += getImageCompareResultsTable()
    html += '<br><br>'
    html += getSkipsTable()
    resultSummaryName = resultsFolder + '\\TestSummary.html'
    outfile = open(resultSummaryName, 'w')
    outfile.write(html)
    outfile.close()
    if(openSummary):
        os.system("start " + resultSummaryName)

def cleanScreenShots(dir):
    if not os.path.isdir(dir):
        print 'Error. unable to clean screenshots, could not find ' + dir
        return
    screenshots = [s for s in os.listdir(dir) if s.endswith('.png')]
    for s in screenshots:
        os.remove(dir + '\\' + s)

def main():
    global resultsFolder
    parser = argparse.ArgumentParser()
    parser.add_argument('-nb', '--nobuild', action='store_true', help='run without rebuilding Falcor and test apps')
    parser.add_argument('-ss', '--showsummary', action='store_true', help='opens testing summary upon completion')
    #bto only skips building falcor and falcor test if theyre already build, if theyre not, it'll still build them
    parser.add_argument('-bto', '--buildtestsonly', action='store_true', help='builds test apps, but not falcor. Overriden by nobuild')
    parser.add_argument('-ctr', '--cleantestresults', action='store_true', help='deletes test results dir if exists')
    args = parser.parse_args()

    cleanScreenShots(debugFolder)
    cleanScreenShots(releaseFolder)

    date = time.strftime("%m-%d-%y")
    resultsFolder += '\\' + date
    if not os.path.isdir(resultsFolder):
        os.makedirs(resultsFolder)
    elif args.cleantestresults:
        shutil.rmtree(resultsFolder, ignore_errors=True)
        #this occasionally gives a permission error, not sure why
        os.makedirs(resultsFolder)

    cleanTests = False
    if not args.nobuild and not args.buildtestsonly:
        callBatchFile(['clean', 'debugd3d12'])
        callBatchFile(['clean', 'released3d12'])
        #TODO, clean other configs
    #only clean individual tests if entire soln not already cleaned and want to build tests
    elif args.buildtestsonly:
        cleanTests = True
 
    if not args.nobuild and not args.buildtestsonly:
        #returns 1 on fail
        if callBatchFile(['build', 'debugd3d12', 'Falcor']):
            buildFail(True, 'Falcor', 'debug3d12')
        if callBatchFile(['build', 'released3d12', 'Falcor']):
            buildFail(True, 'Falcor', 'released3d12')
        if callBatchFile(['build', 'debugd3d12', 'FalcorTest']):
            buildFail(True, 'FalcorTest', 'debugd3d12')
        if callBatchFile(['build', 'released3d12', 'FalcorTest']):
            buildFail(True, 'FalcorTest', 'released3d12')
        #TODO, build other falcor configs, or better, only build configs listed in the test file

    readTestList(not args.nobuild, cleanTests)
    outputHTML(args.showsummary)

if __name__ == '__main__':
    main()