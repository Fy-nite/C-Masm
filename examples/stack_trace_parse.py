# take a stack frame dump from stdin and a dbg file and print functions
import sys
file = sys.argv[1]
dump = input("Enter stack dump: ")[1:-4].split(",")

lbls = []
data = b""
with open(file, "rb") as f:
    data = f.read()

while len(data) != 0:
    string = data[0:data.index(0)]
    data = data[len(string)+1:]
    addr = int.from_bytes(data[0:4], "little")
    data = data[4:]
    lbls.append((string, addr))

lbls.reverse()

for i in dump:
    i = int(i)
    closest = None
    for idx, l in enumerate(lbls):
        d = i-l[1]
        if d < 0:
            closest = lbls[idx-1]
            break
    if closest == None: closest = lbls[-1]
    print(f"{closest[0].decode()}+{i-closest[1]} ({i})")
