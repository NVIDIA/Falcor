import subprocess
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

resultList = []
skippedList = []
resultSummary = TestResults()

def overwriteMove(filename, newLocation):
    shutil.copy(filename, newLocation)
    os.remove(filename)

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

def buildFail(fatal, testName):
    buildFailLog = testName + "_BuildFailLog.txt"
    overwriteMove(buildFailLog, resultsFolder)
    if fatal:
        print 'Fatal error, failed to build ' + testName
        sys.exit(1)
    else:
        logTestSkip(testName, 'Build Failure')

#crash handling needs to be improved, rn entire test just registers a single 
#crash, needs to be able to recover and run not crashing tests, probably gonna
#require some improvements to testbase
def addCrash(testName):
    global resultList
    global resultSummary
    currentResult = TestResults()
    currentResult.Total = 1
    currentResult.Crashed = 1
    resultSummary.add(currentResult)
    resultList.append((testName, currentResult))

def processTestResult(testName):
    global resultList
    global resultSummary
    resultFile = testName + "_TestingLog.xml" 
    if os.path.isfile(resultFile):
        try:
            xmlDoc = minidom.parse(resultFile)
        except ExpatError:
            logTestSkip(testName, resultFile + ' was found, but is not correct xml')
            overwriteMove(resultFile, resultsFolder)
            return
        summary = xmlDoc.getElementsByTagName('Summary')
        if len(summary) != 1:
            print 'Error parsing ' + resultFile + ' found no summary or multiple summaries'
            logTestSkip(testName, resultFile + ' was found and is XML, but is improperly formatted')
            overwriteMove(resultFile, resultsFolder)
            return
        newResult = TestResults()
        newResult.Name = testName
        newResult.Total = int(summary[0].attributes['TotalTests'].value)
        newResult.Passed = int(summary[0].attributes['PassedTests'].value)
        newResult.Failed = int(summary[0].attributes['FailedTests'].value)
        resultList.append((testName, newResult))
        resultSummary.add(newResult)
        overwriteMove(resultFile, resultsFolder)
    else:
        print 'Unable to find result file ' + resultFile
        logTestSkip(testName, 'Unable to find result file ' + resultFile)

def readTestList(buildTests, cleanTests):
    testList = open(testListFile)
    for line in testList.readlines():
        #strip off newline if there is one 
        if line[-1] == '\n':
            line = line[:-1]

        spaceIndex = line.find(' ');
        if spaceIndex == -1:
            print 'Skipping improperly formatted test request: ' + line
            logTestSkip(line, 'Improperly formatted test request')
            continue

        testName = line[:spaceIndex]
        configName = line[spaceIndex + 1:]
        if configName == 'debug' or configName == 'release':
            if buildTests:
                cleanTests
                #returns 1 on fail
                if callBatchFile(['build', configName, testName]):
                    buildFail(False, testName + "_" + configName)
                    continue
            runTest(testName, configName)
        else:
            print 'Skipping test \"' + testName + '\" Unrecognized desired config \"' + configName + '\"'
            logTestSkip(testName + "_" + configName, 'Unrecognized config ' + configName)

def runTest(testName, configName):
    if configName == 'debug':
        testPath = debugFolder + testName + ".exe"
    elif configName == 'release':
        testPath = releaseFolder + testName + ".exe"

    try:
        if os.path.exists(testPath):
            subprocess.check_call([testPath])
            processTestResult(testName + '_' + configName)
        else:
            print 'Skipping test \"' + testName + '\" Unable to find ' + testPath + ' to run'
            logTestSkip(testName + '_' + configName, 'Unable to find ' + testPath)
    except subprocess.CalledProcessError:
        addCrash(testName)

def testResultToHTML(result):
    html = '<tr>'
    html += '<td>' + result.Name + '</td>\n'
    html += '<td>' + str(result.Total) + '</td>\n'
    html += '<td>' + str(result.Passed) + '</td>\n'
    html += '<td>' + str(result.Failed) + '</td>\n'
    html += '<td>' + str(result.Crashed) + '</td>\n'
    html += '</tr>\n'
    return html

def getTestResultsTable():
    html = '<table style="width:100%" border="1">\n'
    html += '<tr>\n'
    html += '<th colspan=\'5\'>Test Results</th>\n'
    html += '</tr>\n'
    html += '<tr>\n'
    html += '<th>Test</th>\n'
    html += '<th>Total</th>\n'
    html += '<th>Passed</th>\n'
    html += '<th>Failed</th>\n'
    html += '<th>Crashed</th>\n'
    resultSummary.Name = 'Summary'
    html += testResultToHTML(resultSummary)
    for name, result in resultList:
        html += testResultToHTML(result)
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
    html += '</tr><tr>'
    html += '<th>Test</th>\n'
    html += '<th>Reason for Skip</th>\n'
    html += '</tr>\n'
    for name, reason in skippedList:
        html += skipToHTML(name, reason)
    html += '</table>'
    return html

def outputHTML(openSummary):
    html = getTestResultsTable()
    html += '<br><br>'
    html += getSkipsTable()
    date = time.strftime("%m-%d-%y")
    resultSummaryName = resultsFolder + '\\TestSummary_' + date  + '.html'
    outfile = open(resultSummaryName, 'w')
    outfile.write(html)
    outfile.close()
    if(openSummary):
        os.system("start " + resultSummaryName)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-nb', '--nobuild', action='store_true', help='run without rebuilding Falcor and test apps')
    parser.add_argument('-ss', '--showsummary', action='store_true', help='opens testing summary upon completion')
    #bto only skips building falcor and falcor test if theyre already build, if theyre not, it'll still build them
    parser.add_argument('-bto', '--buildtestsonly', action='store_true', help='builds test apps, but not falcor. Overriden by nobuild')
    args = parser.parse_args()

    if not os.path.isdir(resultsFolder):
        os.makedirs(resultsFolder)

    cleanTests = False
    if not args.nobuild and not args.buildtestsonly:
        callBatchFile(['clean', 'debug'])
        callBatchFile(['clean', 'release'])
    #only clean individual tests if entire soln not already cleaned and want to build tests
    elif args.buildtestsonly:
        cleanTests = True
 
    if not args.nobuild and not args.buildtestsonly:
        #returns 1 on fail
        if callBatchFile(['build', 'debug', 'Falcor']):
            buildFail(True, 'Falcor_debug')
        if callBatchFile(['build', 'release', 'Falcor']):
            buildFail(True, 'Falcor_release')
        if callBatchFile(['build', 'debug', 'FalcorTest']):
            buildFail(True, 'FalcorTest_debug')
        if callBatchFile(['build', 'release', 'FalcorTest']):
            buildFail(True, 'FalcorTest_release')

    readTestList(not args.nobuild, cleanTests)
    outputHTML(args.showsummary)

if __name__ == '__main__':
    main()