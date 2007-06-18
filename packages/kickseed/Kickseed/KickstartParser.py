import sys
from Kickseed.KickstartData import KickstartData

class KickstartError(Exception): pass
class KickstartNotImplemented(Exception): pass

class KickstartParser:
    def __init__(self):
        self._sections = {}

    def read(self, fd, startsection = 'main'):
        section = startsection
        for line in fd:
            line = line.strip()

            if line == '':
                continue
            elif line.startswith('%include'):
                print >>sys.stderr, \
                      "Note: %include directives that depend on %pre " \
                      "sections are not supported."
                args = line.split()
                if len(args) < 2:
                    raise KickstartError, 'Invalid %include line'
                section = self.read(open(args[1], 'r'), startsection = section)
                continue
            elif line.startswith('%packages'):
                section = 'packages'
            elif line.startswith('%pre'):
                section = 'pre'
            elif line.startswith('%post'):
                section = 'post'

            self._sections[section].append(line)

        return section

    def parse(self, fd):
        self._sections = {}
        for section in 'main', 'packages', 'pre', 'post':
            self._sections[section] = []

        self.read(fd, startsection = 'main')

        data = KickstartData()
        for line in self._sections['main']:
            if line == '':
                continue
            elif line.startswith('#'):
                continue
            else:
                tokens = line.split(None, 1)
                if len(tokens) > 1:
                    data.main[tokens[0]] = tokens[1]
                else:
                    data.main[tokens[0]] = ''

        if self._sections['packages']:
            # TODO: options to %packages
            # TODO: groups

            data.packages = []

            for pkg in self._sections['packages'][1:]:
                if pkg.startswith('@'):
                    # TODO: groups
                    raise KickstartNotImplemented, "Package groups not implemented"
                else:
                    data.packages.append(pkg)

        if self._sections['pre']:
            tokens = self._sections['pre'][0].split()
            data.preline = tokens[1:]
            data.prelist = self._sections['pre'][1:]

        if self._sections['post']:
            tokens = self._sections['post'][0].split()
            data.postline = tokens[1:]
            data.postlist = self._sections['post'][1:]

        return data
