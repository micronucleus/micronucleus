require_relative "../ruby/micronucleus.rb"

# just converts a hex file to a binary file
program = HexProgram.new open ARGV.first

File.open ARGV.first.split('/').last.sub(/\.hex$/, '') + '.raw', 'w' do |file|
  file.write program.binary
  file.close
end
