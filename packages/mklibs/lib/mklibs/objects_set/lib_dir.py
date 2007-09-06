class LibDir(object):
    def __init__(self, root, dir):
        self.root, self.dir = root, dir

    def find_lib(self, name, rpath = None):
        raise NotImplementedError

    def get_objects(self):
        return []

def RPathLibDir(LibDir):
    def find_lib(self, name, rpath = None):
        raise NotImplementedError

