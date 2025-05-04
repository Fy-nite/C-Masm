strs = {
    "RAX":"{(MathOperatorTokenType)1, 0}", "RBX":"{(MathOperatorTokenType)1, 1}", "RCX":"{(MathOperatorTokenType)1, 2}", "RDX":"{(MathOperatorTokenType)1, 3}",
    "RSI":"{(MathOperatorTokenType)1, 4}", "RDI":"{(MathOperatorTokenType)1, 5}", "RBP":"{(MathOperatorTokenType)1, 6}", "RSP":"{(MathOperatorTokenType)1, 7}",
    "R0":"{(MathOperatorTokenType)1, 8}", "R1":"{(MathOperatorTokenType)1, 9}", "R2":"{(MathOperatorTokenType)1, 10}", "R3":"{(MathOperatorTokenType)1, 11}",
    "R4":"{(MathOperatorTokenType)1, 12}", "R5":"{(MathOperatorTokenType)1, 13}", "R6":"{(MathOperatorTokenType)1, 14}", "R7":"{(MathOperatorTokenType)1, 15}",
    "R8":"{(MathOperatorTokenType)1, 16}", "R9":"{(MathOperatorTokenType)1, 17}", "R10":"{(MathOperatorTokenType)1, 18}", "R11":"{(MathOperatorTokenType)1, 19}",
    "R12":"{(MathOperatorTokenType)1, 20}", "R13":"{(MathOperatorTokenType)1, 21}", "R14":"{(MathOperatorTokenType)1, 22}", "R15":"{(MathOperatorTokenType)1, 23}",
    
    "+":"{(MathOperatorTokenType)0, 0}", "-":"{(MathOperatorTokenType)0, 1}", "*":"{(MathOperatorTokenType)0, 2}", "/":"{(MathOperatorTokenType)0, 3}", ">>":"{(MathOperatorTokenType)0, 5}", "<<":"{(MathOperatorTokenType)0, 6}", "|":"{(MathOperatorTokenType)0, 7}", "&":"{(MathOperatorTokenType)0, 8}", "^":"{(MathOperatorTokenType)0, 9}"
}

tree = {}

for i in strs.keys():
    t = tree
    for idx in range(len(i)):
        if i[idx] not in t.keys():
            t[i[idx]] = {}
        t = t[i[idx]]
        
def get_token(k, final, data, others, pre):
    return (pre + "{'{C}', {'{C}', {F}, {D}, {O}}},\n").replace("{C}", k).replace("{F}", final).replace("{D}", data).replace("{O}", (("{\n" + pre + "{O}\n" + pre + "}").replace("{O}", others) if others.strip() != "" else "{}"))

def loop(tree, pre="", data=""):
    ret = ""
    for k, v in tree.items():
        print(k, repr(pre), data)
        final = len(v.keys()) == 0
        data_r = data + k
        ret += get_token(k, "true" if final else "false", strs[data_r] if final else "{(MathOperatorTokenType)3, 0}", loop(v, pre+"    ", data_r), pre)
    return ret

print(tree)
print(loop(tree))