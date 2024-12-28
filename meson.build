project('QMidi', 'cpp')

cpp_compiler = meson.get_compiler('cpp')

qt5 = import('qt5', required: get_option('qt5'))
qt6 = import('qt6', required: get_option('qt6'))

qt5_dep = dependency('qt5', modules: ['Core'], required: get_option('qt5'))
qt6_dep = dependency('qt6', modules: ['Core'], required: get_option('qt6'))

qt5_sources = []
qt6_sources = []

include_dir = []
sources = []
dependencies = []

## Legacy ##

# Common QMidi source files & library
include_dir += include_directories( 'src/legacy')
sources += ['src/legacy/QMidiFile.cpp', 'src/legacy/QMidiIn.cpp', 'src/legacy/QMidiOut.cpp', 'src/QMidiRoster.cpp']

# Common QMidi Qt version specific sources
# Qt version specific sources
if qt5.found()
  qt5_sources += [qt5.preprocess(moc_headers: ['src/legacy/QMidiIn.h'], include_directories: include_dir)]
endif

if qt6.found()
  qt6_sources += [qt6.preprocess(moc_headers: ['src/legacy/QMidiIn.h'], include_directories: include_dir)]
endif

# Platform specific QMidi source files & libraries
if build_machine.system() == 'windows'
  dependencies += cpp_compiler.find_library('winmm')
  sources += 'src/legacy/OS/QMidi_Win32.cpp'

elif build_machine.system() == 'linux'
  dependencies += cpp_compiler.find_library('asound')
  sources += ['src/legacy/OS/QMidi_ALSA.cpp']
  if qt5.found()
    qt5_sources += qt5.preprocess(moc_headers: ['src/legacy/OS/QMidi_ALSA.h'], include_directories: include_dir)
  endif
  if qt6.found()
    qt6_sources += qt6.preprocess(moc_headers: ['src/legacy/OS/QMidi_ALSA.h'], include_directories: include_dir)
  endif

elif build_machine.system() == 'haiku'
  dependencies += cpp_compiler.find_library('midi2')
  sources += 'src/legacy/OS/QMidi_Haiku.cpp'

elif build_machine.system() == 'darwin'
  dependencies += [cpp_compiler.find_library('-framework CoreMIDI'), cpp_compiler.find_library('-framework CoreFoundation'), cpp_compiler.find_library('-framework CoreAudio')]
  sources += 'src/legacy/OS/QMidi_CoreMidi.cpp'

endif

## End Legacy ##

## Modern ##



## End Modern ##


# Add library
if qt5_dep.found()
  library(meson.project_name() + '5', sources + qt5_sources, dependencies: dependencies + qt5_dep, include_directories: include_dir, install: true)
endif

if qt6_dep.found()
  library(meson.project_name() + '6', sources + qt6_sources, dependencies: dependencies + qt6_dep, include_directories: include_dir, install: true)
endif
