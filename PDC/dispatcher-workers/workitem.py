class Workitem(object):
    def __init__(self, itemId, data, name):
        print("Created workitem %s" % itemId)
        self.itemId=itemId
        self.data=data
        self.fromClient=name
        self.result=None
        self.processedBy=None

    def __str__(self):
        return "<Workitem id=%s>" % str(self.itemId)
