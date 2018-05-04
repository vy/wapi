from os.path import join as opj
from os import getenv
from sys import stderr, stdout


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
    BoolVariable('examples', 'Compile examples.', False),
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


### Parse Environment Variables ################################################

env.Append(CCFLAGS = getenv('CFLAGS', ''))
env.Append(CCFLAGS = '-fno-strict-aliasing')
env.Append(LINKFLAGS = getenv('LDFLAGS', ''))


### Library/Header Check #######################################################

common_libs = ['m', 'iw']
common_hdrs = [
    'ctype.h',
    'errno.h',
    'iwlib.h',
    'libgen.h',
    'linux/nl80211.h',
    'math.h',
    'netinet/in.h',
    'net/route.h',
    'stdio.h',
    'stdlib.h',
    'string.h',
    'sys/ioctl.h',
    'sys/socket.h',
    'sys/types.h',
    ]

def CheckPkgConfig(ctx):
     ctx.Message('Checking for pkg-config... ')
     ret = ctx.TryAction('pkg-config pkg-config')[0]
     ctx.Result(ret)
     return ret

def CheckPkg(ctx, pkg, ver):
     ctx.Message('Checking for package %s... ' % pkg)
     ret = ctx.TryAction('pkg-config --atleast-version=%s %s' % (ver, pkg))[0]
     ctx.Result(ret)
     return ret

conf = Configure(
        env,
        custom_tests = {
            'CheckPkgConfig': CheckPkgConfig,
            'CheckPkg': CheckPkg})

def require_lib(lib):
    if not conf.CheckLib(lib):
        Exit(1)

def require_hdr(hdr):
    if not conf.CheckCHeader(hdr):
        Exit(1)

src = env.Clone()	# Library sources.
exa = env.Clone()	# Examples.

if not (env.GetOption('clean') or env.GetOption('help')):
    if env['check']:
        # Checking common libraries.
        map(require_hdr, common_hdrs)
        map(require_lib, common_libs)
        # Check pkg-config.
        if not conf.CheckPkgConfig():
            stderr.write("pkg-config is missing!\n")
            Exit(1)
    # Configuring nl80211.
    if conf.CheckPkg('libnl-1', '1'):
        src.ParseConfig('pkg-config --libs --cflags libnl-1')
        src.Append(CCFLAGS = '-DLIBNL1')
    elif conf.CheckPkg('libnl-2.0', '2'):
        src.ParseConfig('pkg-config --libs --cflags libnl-2.0')
        src.Append(CCFLAGS = '-DLIBNL2')
    elif conf.CheckPkg('libnl-3.0', '3'):
        src.ParseConfig('pkg-config --libs --cflags libnl-3.0')
        src.Append(CCFLAGS = '-DLIBNL3')
    else:
        stderr.write('libnl could not be found!')
        Exit(1)


### Compile WAPI ###############################################################

common_srcs = map(to_src_path, ['util.c', 'network.c', 'wireless.c'])

src.Append(LIBS = common_libs)
src.Append(CPPPATH = [SRCDIR])

src.SharedLibrary(
    opj(LIBDIR, 'wapi'),
    map(src.SharedObject, common_srcs))
src.StaticLibrary(
    opj(LIBDIR, 'wapi_static'),
    map(src.StaticObject, common_srcs)
)


### Compile Examples ###########################################################

if env['examples']:
    exa.Program(opj(EXADIR, 'sample-get.c'), LIBS = ['wapi'])
    exa.Program(opj(EXADIR, 'sample-set.c'), LIBS = ['wapi'])
    exa.Program(opj(EXADIR, 'ifadd.c'), LIBS = ['wapi'])
    exa.Program(opj(EXADIR, 'ifdel.c'), LIBS = ['wapi'])
    exa.Program(opj(EXADIR, 'recover.c'), LIBS = ['wapi'])
    exa.Program(opj(EXADIR, 'hostapd.cpp'))
