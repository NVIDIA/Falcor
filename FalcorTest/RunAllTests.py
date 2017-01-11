import subprocess
import shutil
import time
import os
from xml.dom import minidom
import argparse

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
resultSummary = TestResults()

def buildTest(testName, config):
    subprocess.call([buildBatchFile, config, testName])

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
        xmlDoc = minidom.parse(resultFile)
        summary = xmlDoc.getElementsByTagName('Summary')
        #It makes sense to have this check but it isn't crash friendly, 
        #I think this will stop the entire testing routine if it can't
        #figure out a particular test result file
        if len(summary) != 1:
            print 'Error parsing ' + resultFile + ' found no summary or multiple summaries'
            sys.exit(1)
        newResult = TestResults()
        newResult.Total = int(summary[0].attributes['TotalTests'].value)
        newResult.Passed = int(summary[0].attributes['PassedTests'].value)
        newResult.Failed = int(summary[0].attributes['FailedTests'].value)
        resultList.append((testName, newResult))
        resultSummary.add(newResult)
        shutil.copy(resultFile, resultsFolder)
        os.remove(resultFile)

def readTestList(buildTests):
    testList = open(testListFile)
    for line in testList.readlines():
        #strip off newline if there is one 
        if line[-1] == '\n':
            line = line[:-1]

        spaceIndex = line.find(' ');
        if spaceIndex == -1:
            print 'Skipping improperly formatted test request: ' + line
            continue

        testName = line[:spaceIndex]
        configName = line[spaceIndex + 1:]
        #this would be a good place to eventually check exe exists
        if configName == 'debug' or configName == 'release':
            if buildTests:
                buildTest(testName, configName)
            runTest(testName, configName)
        else:
            print 'Skipping test \"' + testName + '\" Unrecognized desired config \"' + configName + '\"'
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
    except subprocess.CalledProcessError:
        addCrash(testName)

def TestResultToHTML(name, result):
    html = ''
    html += '<tr>'
    html += '<td>' + name + '</td>\n'
    html += '<td>' + str(result.Total) + '</td>\n'
    html += '<td>' + str(result.Passed) + '</td>\n'
    html += '<td>' + str(result.Failed) + '</td>\n'
    html += '<td>' + str(result.Crashed) + '</td>\n'
    html += '</tr>'
    return html

def outputHTML(openSummary):
    html = ''
    html = '<table style="width:100%" border="1">\n'
    html += '<tr>\n'
    html += '<th/>\n'
    html += '<th colspan=\'5\'>TestResults</th>\n'
    html += '</tr>\n'
    html += '<tr>\n'
    html += '<th>Test</th>\n'
    html += '<th>Total</th>\n'
    html += '<th>Passed</th>\n'
    html += '<th>Failed</th>\n'
    html += '<th>Crashed</th>\n'
    html += TestResultToHTML('Summary', resultSummary)
    for name, result in resultList:
        html += TestResultToHTML(name, result)
    html += '</table>'
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
    parser.add_argument('-bto', '--buildtestsonly', action='store_true', help='builds test apps, but not falcor. Overriden by nobuild')
    args = parser.parse_args()

    if not os.path.isdir(resultsFolder):
        os.makedirs(resultsFolder)
 
    if not args.nobuild and not args.buildtestsonly:
        buildTest('Falcor', 'debug')
        buildTest('Falcor', 'release')
        buildTest('FalcorTest', 'debug')
        buildTest('FalcorTest', 'release')

    readTestList(not args.nobuild)
    outputHTML(args.showsummary)

if __name__ == '__main__':
    main()