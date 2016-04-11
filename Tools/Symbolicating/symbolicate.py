# used to manually symbolicate crash reports since Crashlytics does not seem to work for Build Store


import os
import sys
import re
import subprocess

def runCommand(command):
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE)
    #process.wait()
    return process.communicate()[0]

def main():
    ## Open Files ##
    basePath = os.path.dirname(os.path.realpath(__file__))
    f = open(basePath + '/iNDSTemplate.crash', 'r')
    template = f.read()
    f.close()
    crashFile = open(sys.argv[1], 'r')
    crashData = "Last Exception Backtrace:\n"
    lines = crashFile.readlines()
    for line in lines:
        crashData += line.strip() + "\n"
    crashData += "\n"
    crashFile.close()

    ## Create Sym File ##
    symData = re.sub("Last Exception Backtrace:.*?\n\n", crashData, template, flags=re.DOTALL)
    symFile = open(basePath + '/iNDS.crash', 'w')
    symFile.write(symData)
    symFile.close()

    ## Symbolicate ##
    runCommand("DEVELOPER_DIR='/Applications/Xcode.app/Contents/Developer' /Applications/Xcode.app/Contents/SharedFrameworks/DTDeviceKitBase.framework/Versions/A/Resources/symbolicatecrash iNDS.crash iNDS.app.dSYM > iNDS-sym.crash")

    resultFile = open(basePath + '/iNDS-sym.crash', 'r')
    resultData = resultFile.read()
    resultFile.close()

    symbolicatedData = re.findall("Last Exception Backtrace:(.*?)\n\n", resultData, flags=re.DOTALL)[0]

    outFile = open(sys.argv[1] + '.out', 'w')
    outFile.write(symbolicatedData)
    outFile.close()

    os.remove(basePath + '/iNDS.crash')
    os.remove(basePath + '/iNDS-sym.crash')


main()
