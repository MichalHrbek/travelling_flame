# Spojí více csv záznamů do jednoho

from glob import glob
import os
import sys
from main import gen_filename

if __name__ == "__main__":
    if len(sys.argv) == 1:
        dirname = os.path.join(os.path.dirname(__file__), "out/*.csv")
        readlist = list(glob(dirname))
    else:
        readlist = sys.argv[1:]

    fname = gen_filename()
    with open(fname, "w") as wf:
        with open(readlist[0], "r") as rf:
            wf.write(rf.read())
        for i in readlist[1:]:
            with open(i, "r") as rf:
                wf.write("\n".join(rf.readlines()[1:]))