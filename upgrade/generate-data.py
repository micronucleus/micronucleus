#!/usr/bin/python3

# create a c source file from boot.hex ready to include into upgrade.c


import sys


class Ihex: # class to parse an intel hex file

	def __init__( self, filename ):
		self.h = open( filename, 'rb' )
		self.start = 0
		self.size = 0
		self.checksum = 0
		self.data = []

	def getchar( self ): # get one character from intel hex file
		c = chr( self.line[ self.pos ] )
		self.pos += 1
		return c

	def getbyte( self ): # get one (hex)byte from intel hex file
		b = int( self.line[ self.pos : self.pos+2 ], 16 )
		self.pos += 2
		self.checksum += b
		return b

	def getaddress( self ): # get two byte address value (big endian)
		return 256 * self.getbyte() + self.getbyte()

	def getword( self ): # get two byte data (little endian)
		return self.getbyte() + 256 * self.getbyte()

	def readline( self ): # parse one line (returns address, set of data and checksum (0=ok))
		NO = ( None, None, None ) # retval in case of error
		self.line = self.h.readline()
		if len( self.line ) < 11: # minimal ihex line length (01 record type)
			return NO
		self.pos = 0
		if  self.getchar() != ':': # start character
			return NO
		self.checksum = 0
		data = []
		length = self.getbyte() # data byte length
		address = self.getaddress() # destination address
		record = self.getbyte() # record type: 0=data, 1=EOF, 3=address
		if record == 3: # extended address record
			self.segment = self.getaddress() # 0000 for AVR
			self.start = self.getaddress() # hex start address
			address = self.start
		elif record == 0: # data record
			while length:
				data.append( self.getword() )
				length -= 2
		self.getbyte() # checksum byte, (sum of data + checksum) modulo 256 -> 00
		return (address, data, self.checksum & 0xFF)

	def readlines( self ): # parse complete ihex file
		while True:
			address, data, checksum = self.readline()
			if address == None: # end of file or error
				break
			self.data += data
			self.size += len( data )
		return self.data


ihex = Ihex( sys.argv[1] ) # open ihex file
ihex.readlines() # parse the ihex file

f = open( "bootloader_data.c", "w" ) # output file

f.write( '// This file contains the bootloader data itself and the address to install the bootloader\n\n')
f.write( 'const uint16_t bootloader_data[%d] PROGMEM = {\n' % ihex.size)

for word in ihex.data[:-1]: # print all but last data word with a trailing comma
	f.write( '0x%04x, ' % word )
f.write( '0x%04x\n};\n\n' % ihex.data[-1] ) # print the last data word

f.write( "uint16_t bootloader_address = 0x%04X\n" % ihex.start )

f.close()
