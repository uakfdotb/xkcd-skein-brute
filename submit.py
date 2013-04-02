#!/usr/bin/python3

import urllib.parse, urllib.request, sys, subprocess, time

def submit(word):
    url = "http://almamater.xkcd.com/?edu=mit.edu"
    data = urllib.parse.urlencode({'hashable': word})
    binarydata = data.encode('ascii')
    urllib.request.urlopen(url, binarydata)

def handle(line):
    f,hashable,w,d,dist=line.split()
    submit(hashable[1:-1])

def main():
    proc = subprocess.Popen(["./main"],stdout=subprocess.PIPE)
    while True:
        line = proc.stdout.readline()[:-1]
        print(line.decode('ascii'))
        handle(line)
        time.sleep(1)

if __name__=='__main__':
    main()
