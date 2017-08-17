class HexProgram
  def initialize input
    @bytes = Hash.new(0xFF)
    input = input.read if input.is_a? IO
    parse input
  end

  def binary
    bytes.pack('C*')
  end
  
  def bytes
    highest_address = @bytes.keys.max
    
    bytes = Array.new(highest_address + 1) { |index|
      @bytes[index]
    }
  end

  protected

  def parse input_text
    input_text.each_line do |line|
      next unless line.start_with? ':'
      line.chomp!
      length = line[1..2].to_i(16) # usually 16 or 32
      address = line[3..6].to_i(16) # 16-bit start address
      record_type = line[7..8].to_i(16)
      data = line[9... 9 + (length * 2)]
      checksum = line[9 + (length * 2).. 10 + (length * 2)].to_i(16)
      checksum_section = line[1...9 + (length * 2)]

      checksum_calculated = checksum_section.chars.to_a.each_slice(2).map { |slice|
        slice.join('').to_i(16)
      }.reduce(0, &:+)

      checksum_calculated = (((checksum_calculated % 256) ^ 0xFF) + 1) % 256

      raise "Hex file checksum mismatch @ #{line} should be #{checksum_calculated.to_s(16)}" unless checksum == checksum_calculated

      if record_type == 0 # data record
        data_bytes = data.chars.each_slice(2).map { |slice| slice.join('').to_i(16) }
        data_bytes.each_with_index do |byte, index|
          @bytes[address + index] = byte
        end
      end
    end
  end
end
