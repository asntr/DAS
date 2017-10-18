from __future__ import print_function
import sys, time
try:
    import queue
except ImportError:
    import Queue as queue
from collections import defaultdict
import Pyro4.core
from threading import Thread

Pyro4.config.SERIALIZER = 'pickle'
Pyro4.config.SERIALIZERS_ACCEPTED.add('pickle')


def checkerLoop(workRecords, workqueue, item, worker):
    time.sleep(10)
    try:
        curWorker = workRecords[(item.fromClient, item.itemId)]
        if curWorker == worker:
            workRecords.pop((item.fromClient, item.itemId))
            workqueue.put(item)
    except Exception as e:
        pass

class DispatcherQueue(object):
    def __init__(self):
        self.workqueue = queue.Queue()
        self.resultStore = defaultdict()
        self.workRecords = defaultdict()

    @Pyro4.expose
    def putWork(self, item):
        if item.fromClient not in self.resultStore:
            self.resultStore[item.fromClient] = queue.Queue()
        self.workqueue.put(item)

    @Pyro4.expose
    def getWork(self, worker, timeout=5):
        work = self.workqueue.get(timeout=timeout)
        self.workRecords[(work.fromClient, work.itemId)] = worker
        t = Thread(target=checkerLoop, args=(self.workRecords, self.workqueue, work, worker))
        t.daemon = True
        t.start()
        return work

    @Pyro4.expose
    def putResult(self, item, name):
        try:
            if self.workRecords[(item.fromClient, item.itemId)] == name:
                self.workRecords.pop((item.fromClient, item.itemId))
                self.resultStore[item.fromClient].put(item)
        except KeyError as e:
            print(e)

    @Pyro4.expose
    def getResult(self, client, timeout=5):
        return self.resultStore[client].get(timeout=timeout)

    @Pyro4.expose
    def workQueueSize(self):
        return self.workqueue.qsize()



def main():
    # HOST:PORT
    address = str(sys.argv[1]).split(':')
    host = address[0]
    port = int(address[1])

    daemon = Pyro4.core.Daemon(host, port)
    dispatcher = DispatcherQueue()
    uri = daemon.register(dispatcher, "dispatcher")

    print("Dispatcher is running: " + str(uri))
    daemon.requestLoop()

if __name__=="__main__":
    main()