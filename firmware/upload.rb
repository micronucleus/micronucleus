require_relative './microboot'

if ARGV[0]
  if ARGV[0].end_with? '.hex'
    puts "parsing input file as intel hex"
    test_data = HexProgram.new(open ARGV[0]).binary
  else
    puts "parsing input file as raw binary"
    test_data = open(ARGV[0]).read
  end
else
  raise "Pass intel hex or raw binary as argument to script"
end

puts "Plug in programmable device now: (waiting)"
sleep 0.5 while MicroBoot.all.length == 0

thinklet = MicroBoot.all.first
puts "Attached to device: #{thinklet.inspect}"

#puts "Attempting to write '#{test_data.inspect}' to first thinklet's program memory"
#puts "Bytes: #{test_data.bytes.to_a.inspect}"
sleep(0.1) # some time to think?
puts "Attempting to write supplied program in to device's memory"
thinklet.program = test_data

puts "That seems to have gone well! Telling thinklet to run program..."


thinklet.finished # let thinklet know it can go do other things now if it likes
puts "All done!"
