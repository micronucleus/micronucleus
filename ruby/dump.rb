`rm temp-dump.bin`
`avrdude -c usbtiny -p t85 -U flash:r:temp-dump.bin:r`
puts open('temp-dump.bin').read(30).bytes.to_a.map { |x| x.to_s(16) }.join(' ')
