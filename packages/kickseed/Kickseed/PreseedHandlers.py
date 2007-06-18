import string

class PreseedHandlers:
    def __init__(self):
        self.preseeds = []

    def lang(self, args):
        self.preseeds.append(('d-i', 'preseed/locale', 'string', args[0]))

    def keyboard(self, args):
        self.preseeds.append(('d-i', 'console-keymaps-at/keymap', 'select', args[0]))
