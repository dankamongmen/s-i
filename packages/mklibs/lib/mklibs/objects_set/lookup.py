class Lookup(object):
    def __init__(self, path):
        self.path = path

    def find_object(self):
        raise NotImplementedError

    def get_objects(self):
        raise NotImplementedError

