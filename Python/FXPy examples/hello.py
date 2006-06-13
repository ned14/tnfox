#! /usr/bin/env python

import os, sys
sys.path=[os.path.abspath("../../lib/x86_7")]+sys.path # In case TnFOX isn't installed
from TnFOX import *

def runme():
    app = FXApp('Hello', 'Test')
    app.init(len(sys.argv), sys.argv)
    main = FXMainWindow(app, 'Hello', None, None, DECOR_ALL)
    button = FXButton(main, '&Hello, World!', None, app, FXApp.ID_QUIT);
    app.create()
    main.show(PLACEMENT_SCREEN)
    app.run()

if __name__ == '__main__':
    runme()
