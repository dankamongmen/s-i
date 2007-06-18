#! /usr/bin/python

import sys
import Kickseed

def main():
    parser = Kickseed.KickstartParser()
    output = Kickseed.Preseed(parser.parse(open(sys.argv[1], 'r')))
    output.write(sys.stdout)

if __name__ == '__main__':
    main()
