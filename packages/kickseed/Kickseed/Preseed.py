import shlex

from Kickseed.PreseedHandlers import PreseedHandlers

class Preseed:
    def __init__(self, data):
        self.data = data
        self.handlers = PreseedHandlers()

    def write(self, fd):
        for keyword, argstr in self.data.items():
            getattr(self.handlers, keyword)(shlex.split(argstr))

        print >>fd, """#### Modifying syslinux.cfg.

# Edit the syslinux.cfg (or similar) file, and add parameters to the end
# of the append line(s) for the kernel.
#
# You'll at least want to add a parameter telling the installer where to
# get its preseed file from.
# If you're installing from USB media, use this, and put the preseed file
# in the toplevel directory of the USB stick.
#   preseed/file=/hd-media/preseed
# If you're netbooting, use this instead:
#   preseed/url=http://host/path/to/preseed
# If you're remastering a CD, you could use this:
#   preseed/file=/cdrom/preseed
# Be sure to copy this file to the location you specify.
# 
# While you're at it, you may want to throw a debconf/priority=critical in
# there, to avoid most questions even if the preseeding below misses some.
# And you might set the timeout to 1 in syslinux.cfg to avoid needing to hit
# enter to boot the installer.
#
# Language, country, and keyboard selection cannot be preseeded from a file,
# because the questions are asked before the preseed file can be loaded.
# Instead, to avoid these questions, pass some more parameters to the kernel:
#
#    preseed/locale=C
#    console-keymaps-at/keymap=us
#
# Note that the kernel accepts a maximum of 8 command line options and
# 8 environment options (including any options added by default for the
# installer). If these numbers are exceeded, 2.4 kernels will drop any
# excess options and 2.6 kernels will panic.
# Some of the default options, like 'vga=normal', may be safely removed for
# most installations, which may allow you to add more options for
# preseeding.
"""

        qnames = self.handlers.preseeds.keys()
        qnames.sort()
        for qname in qnames:
            (qpackage, qtype, qanswer) = self.handlers.preseeds[qname]
            print >>fd, "%s %s %s %s" % (qpackage, qname, qtype, qanswer)
