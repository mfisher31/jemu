#!/usr/bin/env python
# -*- coding: utf-8 -*-

def juce_includes (project_path):
    return [ '%s/JuceLibraryCode' % project_path, '%s/JuceLibraryCode/modules' % project_path ]

import os, platform
from waflib.Configure import conf

juce_modules = '''
    juce_audio_basics juce_audio_devices juce_audio_formats
    juce_audio_processors juce_core juce_cryptography
    juce_data_structures juce_events juce_graphics juce_gui_basics
    juce_gui_extra juce_opengl
'''

mingw_libs = '''
    uuid wsock32 wininet version ole32 ws2_32 oleaut32
    imm32 comdlg32 shlwapi rpcrt4 winmm gdi32
'''

@conf 
def check_common (self):
    self.check(header_name='stdbool.h', mandatory=True)
    self.check(header_name='stdint.h', mandatory=True)

@conf
def check_mingw (self):
    for l in mingw_libs.split():
        self.check_cxx(lib=l, uselib_store=l.upper())
    self.define('JUCE_PLUGINHOST_VST3', 0)
    self.define('JUCE_PLUGINHOST_VST', 1)
    self.define('JUCE_PLUGINHOST_AU', 0)
    for flag in '-Wno-multichar -Wno-deprecated-declarations'.split():
        self.env.append_unique ('CFLAGS', [flag])
        self.env.append_unique ('CXXFLAGS', [flag])

@conf
def check_mac (self):
    # VST/VST3 OSX Support
    self.define('JUCE_PLUGINHOST_VST3', 0)
    self.define('JUCE_PLUGINHOST_VST', 0)
    self.define('JUCE_PLUGINHOST_AU', 0)

@conf
def check_linux (self):
    return

def get_mingw_libs():
    return [ l.upper() for l in mingw_libs.split() ]

def get_juce_library_code (prefix, extension='.cpp'):
    cpp_only = [ 'juce_analytics' ]
    code = []
    for f in juce_modules.split():
        e = '.cpp' if f in cpp_only else extension
        code.append (prefix + '/include_' + f + e)
    return code
