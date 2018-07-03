#!/usr/bin/env python

''' This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.'''

from subprocess import call, Popen, PIPE
import os, sys

sys.path.append (os.getcwd() + "/tools/waf")
import cross, juce, jemu

VERSION='0.0.1'

def options (opt):
    opt.load ("compiler_c compiler_cxx cross juce")

def configure (conf):
    conf.env.DATADIR = os.path.join (conf.env.PREFIX, 'share/jemu')
    conf.env.PLUGINDIR = os.path.join (conf.env.PREFIX, 'lib/jemu')

    cross.setup_compiler (conf)
    if len(conf.options.cross) <= 0:
        conf.prefer_clang()
    conf.load ("compiler_c compiler_cxx ar cross juce")
    conf.check_cxx_version()

    conf.env.append_unique ('CFLAGS', ['-Wno-deprecated-register'])
    conf.env.append_unique ('CXXFLAGS', ['-Wno-deprecated-register', '-Wno-c++11-narrowing', 
                                         '-Wno-switch', '-Wno-logical-op-parentheses', 
                                         '-Wno-bitwise-op-parentheses', '-Wno-empty-body',
                                         '-Wno-constant-conversion'])

    conf.check_common()
    if cross.is_mingw(conf): conf.check_mingw()
    elif juce.is_mac(): conf.check_mac()
    else: conf.check_linux()

    conf.env.DEBUG = conf.options.debug

    print
    juce.display_header ("Jemu Configuration")
    juce.display_msg (conf, "Installation PREFIX", conf.env.PREFIX)
    juce.display_msg (conf, "Installation DATADIR", conf.env.DATADIR)
    juce.display_msg (conf, "Installation PLUGINDIR", conf.env.PLUGINDIR)
    juce.display_msg (conf, "Debugging Symbols", conf.options.debug)

    print
    juce.display_header ("Compiler")
    juce.display_msg (conf, "CFLAGS", conf.env.CFLAGS)
    juce.display_msg (conf, "CXXFLAGS", conf.env.CXXFLAGS)
    juce.display_msg (conf, "LINKFLAGS", conf.env.LINKFLAGS)

    print (conf.env)

def build_linux_desktop (bld):
    if not juce.is_linux():
        return

    slug = 'jemu'
    src = "tools/linuxdeploy/%s.desktop.in" % (slug)
    tgt = "%s.desktop" % (slug)

    jemu_data = '%s/jemu' % (bld.env.DATADIR)
    jemu_bin  = '%s/bin' % (bld.env.PREFIX)

    if os.path.exists (src):
        bld (features     = "subst",
             source       = src,
             target       = tgt,
             name         = tgt,
             JEMU_BINDIR  = jemu_bin,
             JEMU_DATA    = jemu_data,
             install_path = bld.env.DATADIR + "/applications"
        )

        # bld.install_files (jemu_data, 'src/res/icon.xpm')

def build_juce (bld):
    extension = juce.extension()
    libjuce = bld.shlib (
        features    = 'cxx cxxshlib',
        source      = jemu.get_juce_library_code ('jucer/JuceLibraryCode', extension),
        includes    = jemu.juce_includes ('jucer'),
        env         = bld.env.derive(),
        target      = 'dist/lib/juce',
        name        = 'JUCE',
        uselib      = 'JUCE',
        vnum        = '5.3.2'
    )

    if juce.is_linux():
        libjuce.use = 'ALSA FREETYPE2 X11'
    
    elif juce.is_mac():
        libjuce.install_path = None
        libjuce.linkflags = ['-install_name', '@rpath/libjuce.dylib',
                             '-Wl,-compatibility_version,5.0.0', '-Wl,-current_version,5.3.2']
        libjuce.use = use = 'ACCELERATE AUDIO_TOOLBOX AUDIO_UNIT CORE_AUDIO CORE_AUDIO_KIT \
                             COCOA CORE_MIDI IO_KIT QUARTZ_CORE OPEN_GL WEB_KIT GAME_CONTROLLER'
    
    bld.add_group()
    return libjuce

def copy_mingw_libs (bld):
    return

def build_mingw (bld):
    mingwEnv = bld.env.derive()
    mingwSrc = jemu.get_juce_library_code ("jucer/JuceLibraryCode", ".cpp")
    mingwSrc += bld.path.ant_glob('src/**/*.cpp')
    mingwSrc += bld.path.ant_glob('jucer/JuceLibraryCode/BinaryData*.cpp')
    mingwSrc.sort()
    
    bld.program (
        source      = mingwSrc,
        includes    =  [ ],
        cxxflags    = '',
        target      = '%s/Jemu' % (mingwEnv.CROSS),
        name        = 'Jemu',
        linkflags   = [ '-mwindows', '-static-libgcc', '-static-libstdc++' ],
        use         = jemu.get_mingw_libs(),
        env         = mingwEnv
    )

    bld.add_post_fun (copy_mingw_libs)

def build_linux (bld):
    app = bld.program (
        source = bld.path.ant_glob ('src/**/*.cpp') + bld.path.ant_glob ('src/**/*.mm'),
        includes = jemu.juce_includes('jucer') + [ 'libs/libjemu', 'src' ],
        env = bld.env.derive(),
        target = 'dist/bin/jemu',
        name = 'jemuApp',
        use = [ 'JUCE', 'JEMU', 'X11', 'ALSA', 'GL' ],
    )

    build_linux_desktop (bld)
    return app

def copy_mac_libs (ctx):
    app_binary = 'build/Emulator.app/Contents/MacOS/Emulator'
    plugins = [
        'build/Emulator.app/Contents/Frameworks/plugins/nestopia.jemu/nestopia.dylib',
    ]

    call(['rm', '-rf', 'build/Emulator.app/Contents/Frameworks'])
    call(['mkdir', '-p', 'build/Emulator.app/Contents/Frameworks'])
    call(['rsync', '-ar', '--update', 'build/dist/lib/',
          'build/Emulator.app/Contents/Frameworks/'])
    call(['mv', 'build/Emulator.app/Contents/Frameworks/jemu',
                'build/Emulator.app/Contents/Frameworks/plugins'])
    
    call(['install_name_tool', '-add_rpath', '@loader_path/../Frameworks', app_binary ])
    for plugin in plugins:
        call(['install_name_tool', '-add_rpath', '@loader_path/../..', plugin ])

def build_mac (bld):
    app = bld.program (
        source = bld.path.ant_glob ('src/**/*.cpp') + bld.path.ant_glob ('src/**/*.mm'),
        includes = jemu.juce_includes('jucer') + [ 'libs/libjemu', 'src' ],
        env = bld.env.derive(),
        target = 'Emulator',
        name = 'Emulator',
        use = [ 'JUCE', 'JEMU' ],
        mac_app     = True,
        mac_plist   = 'tools/macdeploy/Info-App.plist',
        # mac_files   = [ 'project/Builds/MacOSX/Icon.icns' ]
    )

    bld.add_post_fun (copy_mac_libs)
    return app

def build (bld):
    build_juce (bld)

    for lib in 'libjemu'.split():
        bld.recurse ('libs/%s' % lib)
        bld.add_group()

    for plugin in 'nestopia mfi'.split():
        bld.recurse ('plugins/%s' % plugin)
        bld.add_group()

    if cross.is_windows (bld):
        return build_mingw (bld)
    elif juce.is_mac():
        return build_mac (bld)
    else:
        build_linux (bld)

def check (ctx):
    call (["build/tests/tests"])
