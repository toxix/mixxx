#!/usr/bin/env python
# -*- coding: utf-8 -*-
import os

# machine-system settings we support
#
# iX86-mingw32-
# iX86-apple-darwin9
# x86_64-apple-darwin9
# powerpc-apple-darwin9
# poperpc64-apple-darwin9
# iX86-linux-gnu
# x86_64-linux-gnu

# platform = [win, osx, bsd, linux]
# machine = [iX86, x86_64, powerpc, powerpc64]
# toolchain = [gnu, msvc]
# target = [debug, release]

# If you don't specify your platform, we autodetect platform/machine using
# python. If you don't specify your machine then we pick iX86 by default
#
# default toolchain: gnu (you'll get an error on Windows if unspecified)
# default build: debug

import logging
logging.getLogger().setLevel(logging.DEBUG)

from build import mixxx, features, depends
target = ARGUMENTS.get('target', None)
machine = ARGUMENTS.get('machine', None)
build_type = ARGUMENTS.get('build', None)
toolchain = ARGUMENTS.get('toolchain', None)

available_features = [features.Mad,
                      features.CoreAudio,
                      features.MediaFoundation,
                      features.HSS1394,
                      features.HID,
                      features.Bulk,
                      features.MacAppStoreException,
                      features.VinylControl,
                      features.Shoutcast,
                      features.Opus,
                      features.Profiling,
                      features.BuildTime,
                      features.QDebug,
                      features.Verbose,
                      features.Optimize,
                      features.FAAD,
                      features.WavPack,
                      features.ModPlug,
                      features.TestSuite,
                      features.Vamp,
                      features.AutoDjCrates,
                      features.ColorDiagnostics,
                      features.AddressSanitizer,
                      features.LocaleCompare,

                      # "Features" of dubious quality
                      features.PerfTools,
                      features.AsmLib,
                      features.IPod,
                      features.FFMPEG,
                      ]

build = mixxx.MixxxBuild(target, machine, build_type,
                         toolchain, available_features)
Export('build')

# Run our configuration stuff to setup our build environment (detect
# platform/arch, find libraries, etc)
SConscript(File('src/SConscript.env'), variant_dir=Dir(build.build_dir), duplicate=0)

# Grab the list of sources generated by SConscript.env
Import('sources')

# Make the main src directory accessible to the SoundSourceM4A, so
# SoundSourceM4A can get at sounddevice.cpp, which it needs to build
# and link properly. This sucks but it's the best way I can find -- bkgood
VariantDir("plugins/soundsourcem4a", "src", duplicate=0)
VariantDir("plugins/soundsourcewv", "src", duplicate=0)
VariantDir("plugins/soundsourcemediafoundation", "src", duplicate=0)

# Build our soundsource plugins
soundsource_plugins = SConscript(
    File('plugins/SConscript'), variant_dir=Dir(build.build_dir + "/plugins"),
    duplicate=0, exports=['build'])

#Build Vamp minimal plugins
#VariantDir("vamp-plugins", "src", duplicate=0)
mixxxminimal_depends = [ depends.SoundTouch ]
mixxxminimal_plugins = SConscript(
    File('vamp-plugins/SConscript'),
    variant_dir=Dir(build.build_dir + "/vamp-plugins"),
    duplicate=0, exports=['build', 'mixxxminimal_depends'])

# Setup and build the "mixxx" executable target. Also sets up our install
# targets for each platform.
SConscript(File('src/SConscript'), variant_dir=Dir(build.build_dir), duplicate=0,
           exports=['build', 'sources', 'soundsource_plugins', 'mixxxminimal_plugins'])

#On OS X, if the bundle has been built, copy it up:
#Command("Mixxx.app", os.path.join(build.build_dir, "Mixxx.app"), Copy("$TARGET", "$SOURCE"))
