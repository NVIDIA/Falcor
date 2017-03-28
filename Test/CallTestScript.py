import subprocess
import argparse
import os
from datetime import date
import RunAllTests

gEmailRecipientFile = 'EmailRecipients.txt'

class TestSetInfo(object):
    def __init__(self, testDir, testList, result):
        self.testDir = testDir
        self.testList = testList
        self.result = result

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

def sendEmail(subject, body, attachment):
    sender = 'clavelle@nvidia.com'
    recipients = str(open(gEmailRecipientFile, 'r').read());
    subprocess.call(['blat.exe', '-install', 'mail.nvidia.com', sender])
    if attachment != None:
        subprocess.call(['blat.exe', '-to', recipients, '-subject', subject, '-body', body, '-attach', attachment])
    else:
        subprocess.call(['blat.exe', '-to', recipients, '-subject', subject, '-body', body])

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

        noBuild = argList[0].strip().lower() != 'true'
        refDir = argList[1].strip()
        testList = argList[2].strip()

        if args.nopull:
            shouldPull = False
        else:
            shouldPull = argList[3].strip().lower() == 'true'

        command = 'python.exe RunAllTests.py -ref ' + refDir + ' -tests ' + testList
        if noBuild:
            command += ' -nb'
        if args.showsummary:
            command += ' -ss'
        if args.generatereference:
            command += ' -gr'

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
        setInfo = TestSetInfo(workingDir, testList, 'Unitialized')
        try:
            print 'Running Test list ' + testList + ' in test dir ' + workingDir
            subprocess.check_call(command)
            setInfo.result = 'Success'
        except:
            setInfo.result = 'Fail'
        testResults.append(setInfo)

        if testDir:
            os.chdir(prevWorkingDir)

        contents = contents[argEndIndex + 1 :]
        argStartIndex = contents.find('{')

    if not args.noemail and not args.generatereference:
        body = 'Ran ' + str(len(testResults)) + ' test sets:\n'
        anyFails = False
        for r in testResults:
            if r.result == 'Fail':
                anyFails = True
            body += r.testDir + '\\' + r.testList + ': ' + r.result + '\n'
        if anyFails:
            subject = '[FAILED]'
        else:
            subject = '[SUCCESS]'
        dateStr = date.today().strftime("%m-%d-%y")
        subject += ' Falcor automated testing ' + dateStr
        sendEmail(subject, body, None)

if __name__ == '__main__':
    main()
