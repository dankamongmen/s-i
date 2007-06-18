import string

class PreseedHandlerException(Exception): pass
class UnimplementedCommand(PreseedHandlerException): pass
class UnimplementedArgument(PreseedHandlerException): pass

class PreseedHandlers:
    def __init__(self):
        self.preseeds = []

    def preseed(self, qpackage, qname, qtype, qanswer):
        self.preseeds.append((qpackage, qname, qtype, qanswer))

    def auth(self, args):
        for arg in args:
            if arg == '--enablemd5':
                self.preseed('passwd', 'passwd/md5', 'boolean', 'true')
            elif arg == '--useshadow' or arg == '--enableshadow':
                # TODO: this is true by default already
                self.preseed('passwd', 'passwd/shadow', 'boolean', 'true')
            elif arg == '--enablecache':
                raise UnimplementedArgument, 'nscd not supported'
            else:
                # TODO --enablenis, --nisdomain=, --nisserver=, --enableldap,
                # --enableldapauth, --ldapserver=, --ldapbasedn=,
                # --enableldaptls, --enablekrb5, --krb5realm=, --krb5kdc=,
                # --krb5adminserver=, --enablehesiod, --hesiodlhs,
                # --hesiodrhs, --enablesmbauth, --smbservers=, --smbworkgroup=
                raise UnimplementedArgument, arg

    def autostep(self, args):
        raise UnimplementedCommand, "autostep makes no sense in d-i"

    def bootloader(self, args):
        for arg in args:
            if arg.startswith('--location='):
                location = arg[11:]
                if location == 'mbr':
                    # TODO: not always hd0; lilo
                    self.preseed('d-i', 'grub-installer/bootdev', 'string', '(hd0)')
                elif location == 'partition':
                    # TODO: lilo
                    self.preseed('d-i', 'grub-installer/bootdev', 'string', '(hd0,1)')
                elif location == 'none':
                    # TODO need lilo-installer/skip too
                    self.preseed('d-i', 'grub-installer/skip', 'boolean', 'true')
                else:
                    raise UnimplementedArgument, arg
            elif arg.startswith('--useLilo'):
                self.preseed('d-i', 'grub-installer/skip', 'boolean', 'true')
            elif arg.startswith('--upgrade'):
                raise UnimplementedArgument, 'upgrades using installer not supported'
            else:
                # TODO --password=, --md5pass=, --linear, --nolinear, --lba32
                raise UnimplementedArgument, arg

    def keyboard(self, args):
        self.preseed('d-i', 'console-keymaps-at/keymap', 'select', args[0])

    def lang(self, args):
        self.preseed('d-i', 'preseed/locale', 'string', args[0])
