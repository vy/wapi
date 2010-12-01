from os.path import join as opj
from os import getenv
from sys import stdout


### Common Variables ###########################################################

INCDIR = 'include'
LIBDIR = 'lib'
SRCDIR = 'src'
EXADIR = 'examples'


### Common Utilities ###########################################################

def to_src_path(file):
    return opj(SRCDIR, file)


### Parse Command Line Arguments ###############################################

vars = Variables()
vars.AddVariables(
    BoolVariable('debug', 'Enable debug symbols.', True),
    BoolVariable('optimize', 'Compile with optimization flags turned on.', True),
    BoolVariable('profile', 'Enable profile information.', False),
    BoolVariable('check', 'Enable library/header checks.', True),
    )
env = Environment(variables = vars)
Help(vars.GenerateHelpText(env))

if env['debug']:
    env.Append(CCFLAGS = '-g')

if env['optimize']:
    env.Append(CCFLAGS = '-O3 -pipe')
    if not env['profile']:
        env.Append(CCFLAGS = '-fomit-frame-pointer')

if env['profile']:
    env.Append(CCFLAGS = '-pg')
    env.Append(LINKFLAGS = '-pg')


### Generic Compiler Flags #####################################################

env.Append(CCFLAGS = '-Wall')
env.Append(CPPPATH = [INCDIR])
env.Append(LIBPATH = LIBDIR)


### Set Compiler Flags #########################################################

env.Append(CCFLAGS = getenv('CFLAGS', ''))
env.Append(LINKFLAGS = getenv('LDFLAGS', ''))


### Library/Header Check #######################################################

common_libs = ['m', 'iw', 'nl', 'nl-genl']
common_hdrs = [
    'ctype.h',
    'errno.h',
    'linux/nl80211.h',
    'math.h',
    'netinet/in.h',
    'netlink/attr.h',
    'netlink/genl/ctrl.h',
    'netlink/genl/family.h',
    'netlink/genl/genl.h',
    'netlink/msg.h',
    'net/route.h',
    'stdio.h',
    'stdlib.h',
    'string.h',
    'sys/ioctl.h',
    'sys/socket.h',
    'sys/types.h',
    ]

if not env.GetOption('clean') and env['check']:
    conf = Configure(env)
    map(conf.CheckLib, common_libs)
    map(conf.CheckCHeader, common_hdrs)


### Compile WAPI ###############################################################

common_srcs = map(to_src_path, ['util.c', 'network.c', 'wireless.c'])

src = env.Clone()
src.Append(CPPPATH = [SRCDIR])

src.SharedLibrary(
    opj(LIBDIR, 'wapi'),
    map(src.SharedObject, common_srcs),
    LIBS = common_libs)


### Compile Examples ###########################################################

exa = env.Clone()
exa.Append(CCFLAGS = '-fno-strict-aliasing')

exa.Program(
    opj(EXADIR, 'sample-get.c'),
    LIBS = common_libs + ["wapi"])

exa.Program(
    opj(EXADIR, 'sample-set.c'),
    LIBS = common_libs + ["wapi"])

exa.Program(
    opj(EXADIR, 'ifadd.c'),
    LIBS = common_libs + ["wapi"])

exa.Program(
    opj(EXADIR, 'ifdel.c'),
    LIBS = common_libs + ["wapi"])
