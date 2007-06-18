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
                raise UnimplementedArgument, arg

    def autostep(self, args):
        raise UnimplementedCommand, "autostep makes no sense in d-i"

    def keyboard(self, args):
        self.preseed('d-i', 'console-keymaps-at/keymap', 'select', args[0])

    def lang(self, args):
        self.preseed('d-i', 'preseed/locale', 'string', args[0])
