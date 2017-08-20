require_relative "./hexprogram.rb"

data = HexProgram.new(open ARGV.first)

#puts data.instance_variable_get(:@bytes).inspect

data = data.bytes

# find start address
start_address = 16 # skip past baked in trampoline - upgrade firmware generates one anyway!
# TODO: Verify jump table? or store it in the upgrade firmware for verbatim installation?
start_address += 1 while data[start_address] == 0xFF

raise "Seems to be junk data quite early in the bootloader" unless start_address > 100

# trim blank padding data from start of data
start_address.times { data.shift }

# if data is an odd number of bytes make it even
data.push 0xFF while (data.length % 2) != 0

#puts "Length: #{data.length}"
#puts "Start address: #{start_address}"
puts "const uint16_t bootloader_data[#{data.length / 2}] PROGMEM = {...};"
puts "uint16_t bootloader_address = #{start_address};"

File.open "bootloader_data.c", "w" do |file|
  file.puts "// This file contains the bootloader data itself and the address to"
  file.puts "// install the bootloader"
  file.puts "// Use generate-data.rb with ruby 1.9 or 2.0 to generate these"
  file.puts "// values from a hex file"
  file.puts "// Generated from #{ARGV.first} at #{Time.now} by #{ENV['USER']}"
  file.puts ""
  file.puts "const uint16_t bootloader_data[#{data.length / 2}] PROGMEM = {"
  file.puts data.each_slice(2).map { |big_end, little_end|
    "0x#{ ((little_end * 256) + big_end).to_s(16).rjust(4, '0') }"
  }.join(', ')
  file.puts "};"
  file.puts ""
  file.puts "uint16_t bootloader_address = #{start_address};"
end
