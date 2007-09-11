class Lookup(object):
    def __init__(self, path):
        import os.path
        if not os.path.exists(path):
            raise RuntimeError

        self.path = path

    def find_object(self):
        return []

    def get_objects(self):
        import os, os.path, mklibs.object

        ret = []

        if os.path.isfile(self.path):
            obj = mklibs.object.get_object(self.path)
            if obj is not None:
                ret.append(obj)

        elif os.path.isdir(self.path):
            for root, dirs, files in os.walk(self.path):
                for f in files:
                    obj = mklibs.object.get_object(os.path.join(root, f))
                    if obj is not None:
                        ret.append(obj)

        else:
            raise NotImplementedError

        return ret

