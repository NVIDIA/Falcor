import subprocess
import shutil
import time
import os
from xml.dom import minidom

testListFile = 'C:\\Users\\clavelle\\Desktop\\FalcorGitHub\\FalcorTest\\TestList.txt'
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
        shutil.move(resultFile, resultsFolder)


def runTests():
    testList = open(testListFile)
    for line in testList.readlines():
        #strip off newline if there is one 
        if line[-1] == '\n':
            line = line[:-1]

        spaceIndex = line.find(' ');
        testName = line[:spaceIndex]
        configName = line[spaceIndex + 1:]

        try:
            if configName == 'Debug':
                subprocess.check_call([debugFolder + testName])
            elif configName == 'Release':
                subprocess.check_call([releaseFolder + testName])
            else:
                print 'Unrecognized desired config \"' + configName + '\" for test \"' + testName + '\", skipping...'
                continue
        except subprocess.CalledProcessError:
            addCrash(testName)
            continue

        processTestResult(testName + '_' + configName)

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

def outputHTML():
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
    outfile = open(resultsFolder + '\\TestSummary_' + date  + '.html', 'w')
    outfile.write(html)

def main():
    if not os.path.isdir(resultsFolder):
        os.makedirs(resultsFolder)

    runTests()
    outputHTML()

if __name__ == '__main__':
    main()