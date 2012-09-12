"""

  This module contains functions for building and loading NMODL mechanisms

  @author Tom Close

"""

#######################################################################################
#
#    Copyright 2012 Okinawa Institute of Science and Technology (OIST), Okinawa, Japan
#
#######################################################################################

import os.path
import shutil
import platform
import subprocess
from ninemlp import DEFAULT_BUILD_MODE

BUILD_ARCHS = [platform.machine(), 'i686', 'x86_64', 'powerpc', 'umac']

if 'NRNHOME' in os.environ:
    os.environ['PATH'] += os.pathsep + os.environ['NRNHOME']
else:
    # I apologise for this hack (this is the path on my machine, to save me having to set the environment variable in eclipse)
    os.environ['PATH'] += os.pathsep + '/opt/NEURON-7.2/x86_64/bin' 

def build (model_dir, build_mode=DEFAULT_BUILD_MODE, verbose=True):
    """
    Builds all NMODL files in a directory
    @param model_dir: The path of the directory to build
    """
    # Change working directory to model directory
    orig_dir = os.getcwd()
    try:
        os.chdir(model_dir)
    except OSError:
        raise Exception("Could not find NMODL directory '%s'" % model_dir)
    # Clean up old build directories if present
    found_lib = False
    for arch in BUILD_ARCHS:
        path = os.path.join(model_dir, arch)
        if os.path.exists(path):
            if build_mode == 'lazy' or build_mode == 'require':
                found_lib = True
            else:
                shutil.rmtree(path, ignore_errors=True)
    print "build_mode: " + build_mode
    print "found_lib: " + str(found_lib)
    if not found_lib:
        if build_mode == 'require':
            raise Exception("The required NMODL binaries were not found in directory '%s' (change the build mode from 'require' any of 'lazy', 'compile_only', or 'force' in order to compile them)." % model_dir)
        # Get platform specific command name
        if platform.system() == 'Windows':
            cmd_name = 'nrnivmodl.exe'
        else:
            cmd_name = 'nrnivmodl'
        # Check the system path for the 'nrnivmodl' command
        cmd_path = None
        for dr in os.environ['PATH'].split(os.pathsep):
            path = os.path.join(dr, cmd_name)
            if os.path.exists(path):
                cmd_path = path
                break
        if not cmd_path:
            raise Exception("Could not find nrnivmodl on the system path '%s'" % os.environ['PATH'])
        print "Building mechanisms in '%s' directory." % model_dir
        if verbose:
            # Run nrnivmodl command on directory
            build_error = subprocess.call(cmd_path)
        else:
            with open(os.devnull, "w") as fnull:
                build_error = subprocess.call(cmd_path, stdout = fnull, stderr = fnull)
        if build_error:
            raise Exception("Could not compile NMODL files in directory '%s' - " % model_dir)
    elif verbose:
        print "Found existing mechanisms in '%s' directory, compile skipped (set 'build_mode' argument to 'force' enforce recompilation them)." % model_dir
    os.chdir(orig_dir)
