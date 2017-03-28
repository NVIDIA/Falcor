import subprocess
import argparse
import os
from datetime import date
import RunAllTests

gEmailRecipientFile = 'EmailRecipients.txt'

class TestSetInfo(object):
    def __init__(self, testDir, testList, summaryFile, errorFile):
        self.testDir = testDir
        self.testList = testList
        self.summaryFile = summaryFile
        self.errorFile = errorFile

def cleanupString(string):
    string = string.replace('\t', '')
    return string.replace('\n', '').strip()

def updateRepo(pullBranch):
    subprocess.call(['git', 'fetch', 'origin', pullBranch])
    subprocess.call(['git', 'checkout', 'origin/' + pullBranch])
    os.chdir('../')
    subprocess.call(['git', 'reset', '--hard'])
    subprocess.call(['git', 'clean', '-fd'])
    os.chdir('test')

def sendEmail(subject, body, attachments):
    sender = 'clavelle@nvidia.com'
    recipients = str(open(gEmailRecipientFile, 'r').read());
    subprocess.call(['blat.exe', '-install', 'mail.nvidia.com', sender])
    command = ['blat.exe', '-to', recipients, '-subject', subject, '-body', body]
    for a in attachments:
        command.append('-attach')
        command.append(a)
    subprocess.call(command)

def main():
    testConfigFile = 'TestConfig.txt'
    parser = argparse.ArgumentParser()
    parser.add_argument('-config', '--testconfig', action='store', help='Allows user to specify test config file')
    parser.add_argument('-np', '--nopull', action='store_true', help='Do not pull/checkout, overriding/ignoring setting in test config')
    parser.add_argument('-ne', '--noemail', action='store_true', help='Do not send emails, overriding/ignoring setting in test config')
    parser.add_argument('-ss', '--showsummary', action='store_true', help='Show a testing summary upon the completion of each test list')
    parser.add_argument('-gr', '--generatereference', action='store_true', help='Instead of running testing, generate reference for each test list')
    args = parser.parse_args()

    if args.testconfig:
        if os.path.exists(args.testconfig):
            testConfigFile = args.testconfig
        else:
            print 'Fatal Error, failed to find user specified test config file ' + args.testconfig

    testConfig = open(testConfigFile)
    contents = file.read(testConfig)
    argStartIndex = contents.find('{')
    testResults = []
    while argStartIndex != -1 :
        testDir = cleanupString(contents[:argStartIndex])
        argEndIndex = contents.find('}')
        argString = cleanupString(contents[argStartIndex + 1 : argEndIndex])
        argList = argString.split(',')
        if len(argList) < 5:
            print 'Error: only found ' + str(len(argList)) + ' args for testing dir ' + testDir + '. Need at least 5 (shouldBuild, refDir, testList, shouldEmail, shouldPull)'
            continue

        shouldBuild = argList[0].strip().lower() == 'true'
        refDir = argList[1].strip()
        testList = argList[2].strip()

        if args.nopull:
            shouldPull = False
        else:
            shouldPull = argList[3].strip().lower() == 'true'

        if testDir:
            prevWorkingDir = os.getcwd()
            os.chdir(testDir)

        if shouldPull:
            try:
                pullBranch = argList[4].strip()
                updateRepo(pullBranch)
            except:
                print 'Error: no pull branch provided for testing dir ' + testDir + ' shouldPull being true. Continuing without pull'

        workingDir = os.getcwd()
        testingResult = RunAllTests.main(shouldBuild, args.showsummary, args.generatereference, refDir, testList)
        #if size 2, includes an error file, means failure
        if len(testingResult) > 1:
            setInfo = TestSetInfo(workingDir, testList, testingResult[0], testingResult[1])
        else:
            setInfo = TestSetInfo(workingDir, testList, testingResult[0], None)
        testResults.append(setInfo)

        if testDir:
            os.chdir(prevWorkingDir)

        contents = contents[argEndIndex + 1 :]
        argStartIndex = contents.find('{')

    if not args.noemail and not args.generatereference:
        body = 'Ran ' + str(len(testResults)) + ' test sets:\n'
        attachments = []
        anyFails = False
        for r in testResults:
            attachments.append(r.summaryFile)
            if r.errorFile:
                anyFails = True
                result = 'Fail'
                attachments.append(r.errorFile)
            else:
                result = 'Success'
            body += r.testDir + '\\' + r.testList + ': ' + result + '\n'
        if anyFails:
            subject = '[FAIL]'
        else:
            subject = '[SUCCESS]'
        dateStr = date.today().strftime("%m-%d-%y")
        subject += ' Falcor automated testing ' + dateStr
        sendEmail(subject, body, attachments)

if __name__ == '__main__':
    main()
