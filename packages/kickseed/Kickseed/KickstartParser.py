class KickstartError(Exception): pass
class KickstartNotImplemented(Exception): pass

class KickstartParser:
    def __init__(self):
        self._sections = {}
        # TODO: data should be a class itself
        self.data = {}

    def read(self, fd, startsection = 'main'):
        section = startsection
        for line in fd:
            line = line.strip()

            if line == '':
                continue
            elif line.startswith('%include'):
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

        self.data = {}
        for line in self._sections['main']:
            if line == '':
                continue
            elif line.startswith('#'):
                continue
            else:
                tokens = line.split(None, 1)
                if len(tokens) > 1:
                    self.data[tokens[0]] = tokens[1]
                else:
                    self.data[tokens[0]] = ''

        if self._sections['packages']:
            # TODO: options to %packages
            # TODO: groups

            self.data['individualPackageList'] = []

            for pkg in self._sections['packages'][1:]:
                if pkg.startswith('@'):
                    # TODO: groups
                    raise KickstartNotImplemented, "Package groups not implemented"
                else:
                    self.data['individualPackageList'].append(pkg)

        if self._sections['pre']:
            tokens = self._sections['pre'][0].split()
            self.data['preLine'] = tokens[1:]
            self.data['preList'] = sections['pre'][1:]

        if self._sections['post']:
            tokens = self._sections['post'][0].split()
            self.data['postLine'] = tokens[1:]
            self.data['postList'] = sections['post'][1:]

        # TODO: I don't like this ...
        return self.data
