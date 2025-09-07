#!/bin/env python

## precompress web resources to be embedded into firmware
# https://github.com/vortigont/FireLamp_JeeUI

#from subprocess import check_output, CalledProcessError
import platform
import os.path
import subprocess
Import("env")

from datetime import datetime
try:
    from zopfli.gzip import compress
    print("found Zopfli compressor, good!")
except:
    from gzip import compress
    print("Using gzip compressor, try to install python zopfli module for better compression")

#print("************** Current CLI targets", COMMAND_LINE_TARGETS)
#print("************** Current Build targets", BUILD_TARGETS)

outdir='embed'
in_gz_files = ["webresources/html/index.html",
               "webresources/html/js/embui.js",
               "webresources/html/js/embui_style.css",
               "webresources/html/js/ui_yo.json",
               "webresources/html/js/ui_embuium.json",
               "webresources/html/js/yo.js",
               "webresources/html/js/yo_style.css",
               "webresources/html/js/yo_theme.css"
               ]
combine_css_files = ["webresources/html/js/yo_theme.css", "webresources/stock_www/style.css"]
static_gz_files = [
    outdir + "/ui_embui.json.gz",
    outdir + "/ui_embui.i18n.json.gz",
    outdir + "/ui_embui.lang.json.gz",
    outdir + "/tz.json.gz",
    outdir + "/lodash.js.gz",
    outdir + "/pure.css.gz"
]

def set_compressor():
    cmd = "where" if platform.system() == "Windows" else "which"
    try:
        subprocess.check_output([cmd, 'zopfli'])
        compressor='zopfli'
        print("found external Zopfli compressor, good!")
    except:
        compressor='internal'

def print_hdr_data(filepath, f_hndlr):
    flat_name = os.path.basename(filepath)
    chars = ".#-/"
    for c in chars:
        flat_name = flat_name.replace(c, "_")
    #f_hndlr.write("#define BLOB_LEN_%s %d\n" % (flat_name, len(gz)))`4`
    f_hndlr.write("extern const uint8_t %s_start[] asm(\"_binary_%s_%s_start\");\n" % (flat_name, outdir, flat_name))
    f_hndlr.write("extern const uint8_t %s_end[] asm(\"_binary_%s_%s_end\");\n" % (flat_name, outdir, flat_name))

def compress_data(input, dst_file):
    try:
        with open(dst_file, 'wb') as f_out:
            f_out.write(compress(input))
    except:
        print("error writing %s" % dst_file)


def compress_file(src_file, f_hndlr):
    try:
        #basename = os.path.basename(src_file)
        #subprocess.check_output(["zopfli", src, "> "],  stdout=outdir + '/' + basename + '.gz')
        with open(src_file, 'rb') as f_in:
            print("*** Compressing %s file..." % src_file)
            fpath = outdir + '/' + os.path.basename(src_file) + '.gz'
            compress_data(f_in.read(), fpath)
            print_hdr_data(fpath, f_hndlr)
    except:
        print("error compressing %s" % src_file)


def embed_resources():
    #if not os.path.exists(outdir):
    #    os.makedirs(outdir)
    # make header file
    with open('include/embed.h', 'wt') as f_hdr:
        f_hdr.write("static const char __BUILD_TSTAMP[] = \"%d\";\n" % int(datetime.now().timestamp()))
        print("*** Compressing embedded resources... ***")

        # compress individual files
        for f in in_gz_files:
            compress_file(f, f_hdr)

        # combine css files
        #cssall = ''.join([open(f, 'r', encoding='utf-8').read() for f in combine_css_files])
        #fpath = outdir + '/yo_style.css.gz'
        #compress_data(cssall, fpath)
        #print_hdr_data(fpath, f_hdr)

        # make header data for static files
        for f in static_gz_files:
            print_hdr_data(f, f_hdr)



#env.AddPreAction("${BUILD_DIR}/src/http.cpp.o", testme)
#env.AddPreAction("buildprog", testme)

embed_resources()
