from getopt import gnu_getopt

class PreseedHandlerException(Exception): pass
class UnimplementedCommand(PreseedHandlerException): pass
class UnimplementedArgument(PreseedHandlerException): pass

class PreseedHandlers:
    def __init__(self):
        self.preseeds = {}

    def _preseed(self, qpackage, qname, qtype, qanswer):
        self.preseeds[qname] = (qpackage, qtype, qanswer)

    def auth(self, args):
        # TODO --enablenis, --nisdomain=, --nisserver=, --enableldap,
        # --enableldapauth, --ldapserver=, --ldapbasedn=,
        # --enableldaptls, --enablekrb5, --krb5realm=, --krb5kdc=,
        # --krb5adminserver=, --enablehesiod, --hesiodlhs,
        # --hesiodrhs, --enablesmbauth, --smbservers=, --smbworkgroup=
        opts = gnu_getopt(args, '',
                          ['enablemd5', 'useshadow', 'enableshadow',
                           'enablecache'])[0]
        for opt, value in opts:
            if opt == '--enablemd5':
                self._preseed('passwd', 'passwd/md5', 'boolean', 'true')
            elif opt == '--useshadow' or opt == '--enableshadow':
                # TODO: this is true by default already
                self._preseed('passwd', 'passwd/shadow', 'boolean', 'true')
            elif opt == '--enablecache':
                raise UnimplementedArgument, 'nscd not supported'
            else:
                raise UnimplementedArgument, opt

    def autostep(self, args):
        raise UnimplementedCommand, "autostep makes no sense in d-i"

    def bootloader(self, args, useLilo = False):
        # TODO --password=, --md5pass=, --linear, --nolinear, --lba32
        opts = gnu_getopt(args, '', ['location=', 'useLilo', 'upgrade'])[0]
        for opt, value in opts:
            if opt == '--location':
                if value == 'mbr':
                    # TODO: not always hd0; lilo
                    self._preseed('d-i', 'grub-installer/bootdev', 'string',
                                  '(hd0)')
                elif value == 'partition':
                    # TODO: lilo
                    self._preseed('d-i', 'grub-installer/bootdev', 'string',
                                  '(hd0,1)')
                elif value == 'none':
                    # TODO need lilo-installer/skip too
                    self._preseed('d-i', 'grub-installer/skip', 'boolean',
                                  'true')
                else:
                    raise UnimplementedArgument, value
            elif opt == 'useLilo':
                useLilo = True
            elif opt == '--upgrade':
                raise UnimplementedArgument, 'upgrades using installer not supported'
            else:
                raise UnimplementedArgument, opt

        if useLilo:
            self._preseed('d-i', 'grub-installer/skip', 'boolean', 'true')

    def clearpart(self, args):
        # TODO --linux, --all
        opts = gnu_getopt(args, '', ['drives=', 'initlabel'])[0]
        for opt, value in opts:
            if opt == '--drives':
                drives = value.split(',')
                if len(drives) > 1:
                    raise UnimplementedArgument, 'clearing multiple drives not supported'
                else:
                    self._preseed('d-i', 'partman-auto/disk', 'string', '/dev/%s' % drives[0])
            elif opt == '--initlabel':
                self._preseed('d-i', 'partman-auto/confirm_write_new_label', 'boolean', 'true')
            else:
                raise UnimplementedArgument, opt

    def device(self, args):
        # type argument (args[0]) ignored
        modulename = args[1]
        opts = gnu_getopt(args[2:], '', ['opts='])[0]
        for opt, value in opts:
            if opt == '--opts':
                # TODO: can't preseed this because hw-detect/module_params
                # is just one question; maybe use db_register?
                raise UnimplementedArgument, "%s %s" % (modulename, opt)
            else:
                raise UnimplementedArgument, opt

    def deviceprobe(self, args):
        # Already the default thanks to hotplug.
        return

    def driverdisk(self, args):
        # TODO <partition> --type=
        raise UnimplementedCommand, 'driver disks not supported'

    def firewall(self, args):
        # TODO <securitylevel> [--trust=] <incoming> [--port=]
        raise UnimplementedCommand, 'firewall preseeding not supported yet'

    def install(self, args):
        # d-i doesn't support upgrades, so this is the default.
        return

    def interactive(self, args):
        # requires debian-installer-utils 1.09, preseed 1.03
        self._preseed('d-i', 'preseed/interactive', 'boolean', 'true')

    def keyboard(self, args):
        self._preseed('d-i', 'console-keymaps-at/keymap', 'select', args[0])

    def lang(self, args):
        self._preseed('d-i', 'preseed/locale', 'string', args[0])

    def langsupport(self, args):
        # TODO REQUIRED <languages> [--default=]
        # preseed/localechooser changes needed to allow different
        # installation and installed languages
        raise UnimplementedCommand, 'langsupport not supported yet'

    def lilo(self, args):
        return self.bootloader(args, useLilo = True)

    def lilocheck(self, args):
        raise UnimplementedCommand, 'lilocheck not supported'

    def logvol(self, args):
        # possible but complex
        # TODO <mountpoint> --vgname=name --size=size --name=name
        raise UnimplementedCommand, 'logvol not supported yet'

    def mouse(self, args):
        if not args:
            # autodetection is the default
            return

        (opts, mousetype) = gnu_getopt(args, '',
                                       ['--device=', '--emulthree'])[0]
        for opt, value in opts:
            if opt == '--device':
                self._preseed('xserver-xorg',
                              'xserver-xorg/config/inputdevice/mouse/port',
                              'select', '/dev/%s' % value)
            elif opt == '--emulthree':
                self._preseed('xserver-xorg',
                              'xserver-xorg/config/inputdevice/mouse/emulate3buttons',
                              'boolean', 'true')
            else:
                raise UnimplementedArgument, opt

            # TODO: translate protocol into xserver-xorg's naming scheme
