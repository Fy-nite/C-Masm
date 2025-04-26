import os
import sys
import subprocess

directory = "/".join(sys.argv[0].split("/")[:-1])
if directory != "": os.chdir(directory)
global failed_code
failed_code = ""

os.mkdir("tmp")

def failed(step, msg):
    global failed_code
    failed_code = (" " * 10) + f"\x1B[1;31m{step} FAILED:\x1b[0m " + msg + "\n"

def success(test):
    print(f"\x1B[1m\x1b[38;5;48mTest {test} succeded!\x1b[0m")


def run_tests():
    print()
    result = os.system("./build.sh > tmp/last_cmd 2>&1")
    if result != 0:
        failed("BUILDING", f"make failed with code {result}, see above")
        return
    success("compile masm")
    
    result = os.system("tmp/masm -c code/hello.masm tmp/hello.bin > tmp/last_cmd 2>&1")
    if result != 0:
        failed("COMPILATION", f"compiler failed with code {result}, see above")
        return
    with open("tmp/hello.bin", "rb") as code:
        with open("code/hello.expected", "rb") as expected:
            if code.read() != expected.read():
                failed("COMPILATION", f"Compiler output is different than expected")
                os.system("tmp/masm -u tmp/hello.bin")
                return
    success("compile hello.masm")
    
    result = os.system("tmp/masm -i tmp/hello.bin > tmp/last_cmd 2>&1")
    if result != 0:
        failed("RUNNING", f"runner failed with code {result}, see above")
        return
    with open("tmp/last_cmd", "r") as f:
        if f.read() != "Hello, World!\n":
            failed("RUNNING", f"runner output is different than expected")
    success("run hello.masm")
    print()
    return 1

i = run_tests()
if i != 1:
    with open("tmp/last_cmd", "r") as f:
        print(f.read())
    print(failed_code)

else:
    print((" " * 10) + f"\x1B[1m\x1b[38;5;49mTESTS PASSED\x1b[0m\n")

for i in os.listdir("tmp"):
    os.remove("tmp/" + i)
os.rmdir("tmp")