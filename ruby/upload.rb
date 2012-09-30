require_relative './micronucleus'

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

#test_data += ("\xFF" * 64)

puts "Plug in programmable device now: (waiting)"
sleep 0.25 while Micronucleus.all.length == 0

nucleus = Micronucleus.all.first
puts "Attached to device: #{nucleus.inspect}"

#puts "Attempting to write '#{test_data.inspect}' to first thinklet's program memory"
#puts "Bytes: #{test_data.bytes.to_a.inspect}"
sleep(0.25) # some time to think?
puts "Attempting to write supplied program in to device's memory"
nucleus.program = test_data

puts "Great! Starting program..."


nucleus.finished # let thinklet know it can go do other things now if it likes
puts "All done!"
