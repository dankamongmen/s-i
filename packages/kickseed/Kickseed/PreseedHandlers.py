from getopt import gnu_getopt

class PreseedHandlerException(Exception): pass
class UnimplementedCommand(PreseedHandlerException): pass
class UnimplementedArgument(PreseedHandlerException): pass
class CommandSyntaxError(PreseedHandlerException): pass

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
        # TODO initrd-preseed
        self._preseed('d-i', 'preseed/interactive', 'boolean', 'true')

    def keyboard(self, args):
        # TODO non-AT
        # TODO initrd-preseed
        self._preseed('d-i', 'console-keymaps-at/keymap', 'select', args[0])

    def lang(self, args):
        # TODO initrd-preseed
        self._preseed('d-i', 'preseed/locale', 'string', args[0])

    def langsupport(self, args):
        # TODO REQUIRED <languages> [--default=]
        # TODO initrd-preseed
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

        (opts, mousetype) = gnu_getopt(args, '', ['device=', 'emulthree'])
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

    def network(self, args):
        statics = {}

        opts = gnu_getopt(args, '',
                          ['bootproto=', 'device=', 'ip=', 'gateway=',
                           'nameserver=', 'nodns', 'netmask=', 'hostname='])[0]
        for opt, value in opts:
            if opt == '--bootproto':
                if value == 'dhcp' or value == 'bootp':
                    pass
                elif value == 'static':
                    self._preseed('d-i', 'netcfg/disable_dhcp', 'boolean',
                                  'true')
                else:
                    raise UnimplementedArgument, value
            elif opt == '--device':
                self._preseed('d-i', 'netcfg/choose_interface', 'select',
                              value)
            elif opt == '--ip':
                self._preseed('d-i', 'netcfg/get_ipaddress', 'string', value)
                statics['ipaddress'] = 1
            elif opt == '--gateway':
                self._preseed('d-i', 'netcfg/get_gateway', 'string', value)
                statics['gateway'] = 1
            elif opt == '--nameserver':
                self._preseed('d-i', 'netcfg/get_nameservers', 'string', value)
                statics['nameservers'] = 1
            elif opt == '--nodns':
                self._preseed('d-i', 'netcfg/get_nameservers', 'string', '')
                statics['nameservers'] = 1
            elif opt == '--netmask':
                self._preseed('d-i', 'netcfg/get_netmask', 'string', value)
                statics['netmask'] = 1
            elif opt == '--hostname':
                self._preseed('d-i', 'netcfg/get_hostname', 'string', value)

        # If all the information displayed on the netcfg/confirm_static
        # screen was preseeded, skip it.
        if ('ipaddress' in statics and 'netmask' in statics and
            'gateway' in statics and 'nameservers' in statics):
            self._preseed('d-i', 'netcfg/confirm_static', 'boolean', 'true')

    def part(self, args):
        return self.partition(args)

    def partition(self, args):
        if 'partman-auto/expert_recipe' in self.preseeds:
            qpackage, qtype, recipe = \
                self.preseeds['partman-auto/expert_recipe']
        else:
            recipe = 'Kickstart-supplied partitioning scheme :'

        size = None
        grow = 0
        # partman-auto doesn't support unlimited-size partitions, so use an
        # arbitrary maximum of one petabyte
        maxsize = 1024 * 1024 * 1024
        format = 1
        asprimary = 0
        fstype = None

        (opts, rest) = gnu_getopt(args, '',
                                  ['size=', 'grow', 'maxsize=',
                                   'noformat', 'onpart=', 'usepart=',
                                   'ondisk=', 'ondrive=', 'asprimary',
                                   'fstype=', 'start=', 'end='])

        for opt, value in opts:
            if opt == '--size':
                size = int(value)
            elif opt == '--grow':
                grow = 1
            elif opt == '--maxsize':
                maxsize = int(value)
            elif opt == '--noformat':
                format = 0
            elif opt == '--onpart' or opt == '--usepart':
                raise UnimplementedArgument, "unsupported restriction 'onpart'"
            elif opt == '--ondisk' or opt == '--ondrive':
                raise UnimplementedArgument, "unsupported restriction 'ondisk'"
            elif opt == '--asprimary':
                asprimary = 1
            elif opt == '--fstype':
                fstype = value
            elif opt == '--start':
                raise UnimplementedArgument, "unsupported restriction 'start'"
            elif opt == '--end':
                raise UnimplementedArgument, "unsupported restriction 'end'"
            else:
                raise UnimplementedArgument, opt

        if len(rest) != 1:
            raise CommandSyntaxError, "partition command requires a mountpoint"
        mountpoint = rest[0]

        if size is None:
            raise CommandSyntaxError, "partition command requires a size"

        if mountpoint == 'swap':
            filesystem = 'swap'
            mountpoint = None
        elif fstype:
            filesystem = fstype
        else:
            filesystem = 'ext3'

        if grow:
            priority = size
        else:
            priority = maxsize
        recipe += ' %u %u %u %s' % (size, priority, maxsize, filesystem)

        if asprimary:
            recipe += ' $primary{ }'

        if filesystem == 'swap':
            recipe += ' method{ swap }'
        elif format:
            recipe += ' method{ format }'
        else:
            recipe += ' method{ keep }'

        if format:
            recipe += ' format { }'

        if filesystem != 'swap':
            recipe += ' use_filesystem{ }'
            recipe += ' filesystem{ %s }' % filesystem

        if mountpoint:
            recipe += ' mountpoint{ %s }' % mountpoint

        recipe += ' .'

        self._preseed('d-i', 'partman-auto/expert_recipe', 'string', recipe)

    def raid(self, args):
        # possible but complex
        raise UnimplementedCommand, 'raid not supported yet'

    def reboot(self, args):
        self._preseed('d-i', 'prebaseconfig/reboot_in_progress', 'note', '')

    def rootpw(self, args):
        # TODO REQUIRED [--iscrypted] <password>
        # requires password preseeding support in shadow (security
        # considerations), or password question in first stage
        raise UnimplementedCommand, 'rootpw not supported yet'

    def skipx(self, args):
        # TODO how do we do this? just don't start gdm?
        raise UnimplementedCommand, 'skipx not supported yet'

    def text(self, args):
        # TODO need to set DEBIAN_FRONTEND=text; how?
        raise UnimplementedCommand, 'text not supported yet'

    def timezone(self, args):
        # TODO REQUIRED [--utc] <timezone>
        # possible to implement as it stands, but very painful; much easier
        # to fix tzsetup to look at a question in zone.tab format.
        raise UnimplementedCommand, 'timezone not supported yet'

    def upgrade(self, args):
        raise UnimplementedCommand, 'upgrades using installer not supported'
