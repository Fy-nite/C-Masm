import os
import sys
import json
import subprocess
import time
import io
import threading

MAX_THREADS = -1

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
    return params[0] == stdout.decode("utf-8")

def file(stdout, stderr, proc, params, test):
    try:
        f1 = params[0]
        f2 = params[1]
        with open(f1, "r") as file1:
            with open(f2, "r") as file2:
                return file1.read() == file2.read()
    except Exception as e:
        return False

def car(prgm):
    ret = [{
            "name": f"compile {prgm}.masm",
            "type": "COMPILING",
            "id": 3,
            "cmd": ["%masm%", "-c", f"%data%/{prgm}.masm", f"%tmp%/{prgm}.bin"],
            "depends": [0],
            "result": [
                {
                    "err":"Masm -c returned non 0 exit code. See Above",
                    "check": "Texit_code",
                    "args":[0]
                },
                {
                    "err":"Masm byte code is wrong. See Below",
                    "check": "Tfile",
                    "args": [f"%tmp%/{prgm}.bin", f"%data%/{prgm}.expected"],
                    "run": ["%masm%", "-u", f"%tmp%/{prgm}.bin"]
                }
            ]
        },
        {
            "name": f"run {prgm}.masm",
            "type": "RUNNING",
            "id": 4,
            "depends": [3],
            "cmd": ["%masm%", "-i", f"%tmp%/{prgm}.bin"],
            "result": [
                {
                    "err":"Masm returned non 0 exit code. See Above",
                    "check": "Texit_code",
                    "args":[0]
                }
            ]
        }]
    return ret

completed_tests = []
failed_tests = []
checks = {
    "exit_code": exit_code,
    "file": file,
    "stdout": stdout_test,
}

macros = {
    "compile_and_run": car
}

vars = {
    "tests": ".",
    "data": "code",
    "tmp": "tmp",
}

def parse(string, extra={}):
    if type(string) is not str:
        return string
    for k,v in vars.items():
        string = string.replace(f"%{k}%", v)
    for k,v in extra.items():
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

    stdout_text = proc.stdout.read()
    stderr_text = proc.stderr.read()

    for i in result:
        res = checks[i["check"][1:]](stdout_text, stderr_text, proc, [parse(a) for a in i["args"]], test)
        if not res and i["check"][0] == "T":
            passed = False
            failed(name, parse(i["err"], {"stdout": repr(stdout_text)}))
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

def first_id(used_ids):
    i = 0
    while True:
        if i not in used_ids:
            return i
        i += 1

def run_tests():
    print("─"*50)

    tests = []
    with open("tests.json", "r") as f:
        tests = json.load(f)["tests"]
    threads = []
    tests_good = []
    used_ids = []
    for i in tests:
        if "macro" in i.keys():
            new = macros[i["macro"][0]](*i["macro"][1:])
            id_lookup = {}
            for b in new:
                id = b["id"]
                b["id"] = first_id(used_ids)
                used_ids.append(b["id"])
                id_lookup[id] = b["id"]
                b["depends"] = [id_lookup.get(g, g) for g in b.get("depends", [])]
                tests_good.append(b)
        else:
            used_ids.append(i["id"])
            tests_good.append(i)
    for i in tests_good:
        print(i)
        t = threading.Thread(target=run_test, args=(i,))
        t.start()
        threads.append(t)
        
    while True in [i.is_alive() for i in threads]:
        time.sleep(0.1)

    print("─"*50)
    return 1

i = run_tests()

#print((" " * 10) + f"\x1B[1m\x1b[38;5;49mTESTS PASSED\x1b[0m\n")

if not "--keep-tmp" in sys.argv:
    for i in os.listdir("tmp"):
        os.remove("tmp/" + i)
    os.rmdir("tmp")