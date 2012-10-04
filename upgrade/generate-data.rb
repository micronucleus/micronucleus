require_relative "../ruby/micronucleus.rb"

data = HexProgram.new(open ARGV.first)

puts data.instance_variable_get(:@bytes).inspect

data = data.bytes

# find start address
start_address = 0
start_address += 1 while data[start_address] == 0xFF

# trim blank padding data from start of data
start_address.times { data.shift }

# if data is an odd number of bytes make it even
data.push 0xFF while (data.length % 2) != 0

puts "Length: #{data.length}"
puts "Start address: #{start_address}"

File.open "bootloader_data.c", "w" do |file|
  file.puts "// This file contains the bootloader data itself and the address to install the bootloader at"
  file.puts "// Use generate-data.rb with ruby 1.9 to generate these values from a hex file"
  file.puts "// Generated from #{ARGV.first} at #{Time.now}"
  file.puts ""
  file.puts "uint16_t bootloader_data[#{data.length / 2}] PROGMEM = {"
  file.puts data.each_slice(2).map { |big_end, little_end|
    "0x#{ ((little_end * 256) + big_end).to_s(16).rjust(4, '0') }"
  }.join(', ')
  file.puts "};"
  file.puts ""
  file.puts "uint16_t bootloader_address = #{start_address};"
end