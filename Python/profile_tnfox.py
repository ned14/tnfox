#! /usr/bin/python
# Copyright 2004 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
import hotshot
import hotshot.stats
import create_tnfox
from environment import settings

if __name__ == '__main__':
    statistics_file = os.path.join( settings.working_dir, 'profile.stat' )
    profile = hotshot.Profile(statistics_file)
    extmodule = create_tnfox.prepare_module()
    profile.runcall( create_tnfox.write_module, extmodule )
    profile.close()
    statistics = hotshot.stats.load( statistics_file )
    #statistics.strip_dirs()
    statistics.sort_stats( 'time', 'cumulative', 'calls' )
    statistics.print_stats( 50 ) 
