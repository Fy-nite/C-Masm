import os
import sys
import json
import subprocess
import time
import io

directory = "/".join(sys.argv[0].split("/")[:-1])
if directory != "": os.chdir(directory)

try:
    os.mkdir("tmp")
except Exception:
    pass

def failed(test, reason):
    print(f"\x1B[1m\x1b[38;5;196mTest {test} failed:\x1b[0m {reason}")

def success(test):
    print(f"\x1B[1m\x1b[38;5;48mTest {test} succeded!\x1b[0m")

def exit_code(stdout, stderr, proc, params, test):
    return params[0] == proc.returncode

def stdout_test(stdout, stderr, proc, params, test):
    return params[0] == stdout.read().decode("utf-8")

def file(stdout, stderr, proc, params, test):
    try:
        f1 = params[0]
        f2 = params[1]
        with open(f1, "r") as file1:
            with open(f2, "r") as file2:
                return file1.read() == file2.read()
    except Exception as e:
        return False

completed_tests = []
failed_tests = []
checks = {
    "exit_code": exit_code,
    "file": file,
    "stdout": stdout_test,
}

vars = {
    "tests": ".",
    "data": "code",
    "tmp": "tmp",
}

def parse(string):
    if type(string) is not str:
        return string
    for k,v in vars.items():
        string = string.replace(f"%{k}%", v)
    return string

def run_test(test: dict):
    name:   str  = test.get("name", "NONE")
    type:   str  = test.get("type", "NONE")
    id:     int  = test.get("id", "NONE")
    cmd:    list = test.get("cmd", "NONE")
    result: dict = test.get("result", "NONE")

    depends: list = test.get("depends", []) # optional

    if name == "NONE":
        failed(id, f"No name")
        failed_tests.append(test)
        return 0
    elif id == "NONE":
        failed(name, f"No id")
        failed_tests.append(test)
        return 0
    elif type == "NONE":
        failed(name, f"No type")
        failed_tests.append(test)
        return 0
    elif cmd == "NONE":
        failed(name, f"No cmd")
        failed_tests.append(test)
        return 0
    elif result == "NONE":
        failed(name, f"No result")
        failed_tests.append(test)
        return 0
    
    if True in [i in [t["id"] for t in failed_tests] for i in depends]: # if depends on failed test
        failed_tests.append(test)
        failed(name, "Depends on failed test")
        return 0
    while False in [i in [t["id"] for t in completed_tests] for i in depends]:
        time.sleep(0.1)
    
    proc = subprocess.Popen([parse(i) for i in cmd], stderr=subprocess.PIPE, stdout=subprocess.PIPE)

    proc.wait()

    passed = True

    for i in result:
        res = checks[i["check"][1:]](proc.stdout, proc.stderr, proc, [parse(a) for a in i["args"]], test)
        if not res and i["check"][0] == "T":
            passed = False
            failed(name, i["err"])
            if "run" in i.keys():
                subprocess.Popen([parse(c) for c in i["run"]]).wait()
            break

    if passed:
        for k,v in test.get("define", {}).items():
            vars[k] = parse(v)
        success(test["name"])
        completed_tests.append(test)
        return 1
    else:
        failed_tests.append(test)
        return 0


def run_tests():
    print("─"*50)

    tests = []
    with open("tests.json", "r") as f:
        tests = json.load(f)["tests"]
    for i in tests:
        run_test(i)
    
    print("─"*50)
    return 1

i = run_tests()

#print((" " * 10) + f"\x1B[1m\x1b[38;5;49mTESTS PASSED\x1b[0m\n")

if not "--keep-tmp" in sys.argv:
    for i in os.listdir("tmp"):
        os.remove("tmp/" + i)
    os.rmdir("tmp")