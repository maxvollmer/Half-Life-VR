#!/usr/bin/python

import sys, getopt, os

def main(argv):
	inputfile = ''

	try:
		opts, args = getopt.getopt(argv, "i:",["ifile="])
		for opt, arg in opts:
			if opt in ("-i", "--ifile"):
				inputfile = arg
	except getopt.GetoptError:
		print('DLLIncludeFixer.py -i <inputfile>')
		sys.exit(2)

	if not os.path.exists(inputfile):
		print('DLLIncludeFixer.py -i <inputfile>')
		sys.exit(2)

	print('Modifying DLL includes in ' + inputfile)

	f = open(inputfile, 'rb')
	s = f.read()
	f.close()

	s = s.replace(b'steam_api.dll', b'steamapi2.dll')

	f = open(inputfile, 'wb')
	f.write(s)
	f.close()

if __name__ == "__main__":
   main(sys.argv[1:])
