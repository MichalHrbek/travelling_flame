from glob import glob
import os
from main import gen_filename

dirname = os.path.join(os.path.dirname(__file__), "out/*.csv")
fname = gen_filename()

readlist = list(glob(dirname))

with open(fname, "w") as wf:
    with open(readlist[0], "r") as rf:
        wf.write(rf.read())
    for i in readlist[1:]:
        with open(i, "r") as rf:
            wf.write("\n".join(rf.readlines()[1:]))