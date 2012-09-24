require 'libusb'

# Abstracts access to uBoot avr tiny85 bootloader - can be used only to upload bytes
class MicroBoot
  Functions = [
    :get_info,
    :write_page,
    :erase_application,
    :run_program
  ]
  
  # return all thinklets
  def self.all
    usb = LIBUSB::Context.new
    usb.devices.select { |device|
      device.product == 'uBoot'
    }.map { |device|
      self.new(device)
    }
  end

  def initialize devref
    @device = devref
  end

  def info
    unless defined? @info
      result = control_transfer(function: :get_info, dataIn: 4)
      flash_length, page_size, write_sleep = result.unpack('S>CC')

      @info = {
        flash_length: flash_length,
        page_size: page_size,
        pages: (flash_length.to_f / page_size.to_f).ceil,
        write_sleep: write_sleep.to_f / 1000.0,
        version: "#{@device.bcdDevice >> 8}.#{@device.bcdDevice & 0xFF}",
        version_numeric: @device.bcdDevice
      }
    end
    @info
  end
  
  def erase!
    puts "Erasing chip..."
    info = self.info
    control_transfer(function: :erase_application)
    
    info[:pages].times do |index|
      puts "Erasing: #{((index.to_f / info[:pages].to_f) * 100.0).round}%" if index % 5 == 0
      sleep(info[:write_sleep]) # sleep for as many pages as the chip has
    end
  end
  
  # upload a new program
  def program= bytestring
    info = self.info
    raise "Program too long!" if bytestring.bytesize > info[:flash_length]
    bytes = bytestring.bytes.to_a
    
    erase!
    
    address = 0
    bytes.each_slice(info[:page_size]) do |slice|
      puts "Uploading: #{(address.to_f / bytes.length.to_f * 100.0).round}%: @#{address} of #{bytes.length}"
      control_transfer(function: :write_page, wIndex: address, wValue: slice.length, dataOut: slice.pack('C*'))
      sleep(info[:write_sleep])
      address += slice.length
    end
  end
  
  def finished
    puts "Asking device to finish writing program..."
    control_transfer(function: :run_program)
    
    # this could be shorter, relative to how many pages we uploaded..
    info[:pages].times do |index|
      puts "Finishing Upload: #{((index.to_f / info[:pages].to_f) * 100.0).round}%" if index % 5 == 0
      sleep(info[:write_sleep]) # sleep for as many pages as the chip has
    end
    
    @io.close
    @io = nil
  end

  protected
  # raw opened device
  def io
    unless @io
      @io = @device.open
    end

    @io
  end
  
  def control_transfer(opts = {})
    opts[:bRequest] = Functions.index(opts.delete(:function)) if opts[:function]
    io.control_transfer({
      wIndex: 0,
      wValue: 0,
      bmRequestType: usb_request_type(opts),
      timeout: 5000
    }.merge opts)
  end



  def usb_request_type opts
    c = LIBUSB::Call
    value = c::RequestTypes[:REQUEST_TYPE_VENDOR] | c::RequestRecipients[:RECIPIENT_DEVICE]
    value |= c::EndpointDirections[:ENDPOINT_OUT] if opts.has_key? :dataOut
    value |= c::EndpointDirections[:ENDPOINT_IN] if opts.has_key? :dataIn
    return value
  end
end

class HexProgram
  def initialize input
    @bytes = Hash.new(0xFF)
    input = input.read if input.is_a? IO
    parse input
  end
  
  def binary
    highest_address = @bytes.keys.max
    
    bytestring = Array.new(highest_address + 1) { |index|
      @bytes[index]
    }.pack('C*')
  end
  
  protected
  
  def parse input_text
    input_text.each_line do |line|
      next unless line.start_with? ':'
      line.chomp!
      length = line[1..2].to_i(16) # usually 16 or 32
      address = line[3..6].to_i(16) # 16-bit start address
      record_type = line[7..8].to_i(16)
      data = line[9.. 9 + (length * 2)]
      checksum = line[9 + (length * 2).. 10 + (length * 2)].to_i(16)
      checksum_section = line[1...9 + (length * 2)]
      
      checksum_calculated = checksum_section.chars.to_a.each_slice(2).map { |slice|
        slice.join('').to_i(16)
      }.reduce(0, &:+)
      
      checksum_calculated = (((checksum_calculated % 256) ^ 0xFF) + 1) % 256
      
      raise "Hex file checksum mismatch @ #{line}" unless checksum == checksum_calculated
      
      if record_type == 0 # data record
        data_bytes = data.chars.each_slice(2).map { |slice| slice.join('').to_i(16) }
        data_bytes.each_with_index do |byte, index|
          @bytes[address + index] = byte
        end
      end
    end
  end
end

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

puts "Finding devices"
thinklets = MicroBoot.all
puts "Found #{thinklets.length} thinklet"
exit unless thinklets.length > 0

thinklet = thinklets.first

puts "First thinklet: #{thinklet.info.inspect}"


puts "Attempting to write '#{test_data.inspect}' to first thinklet's program memory"
puts "Bytes: #{test_data.bytes.to_a.inspect}"
thinklet.program = test_data

puts "That seems to have gone well! Telling thinklet to run program..."


thinklet.finished # let thinklet know it can go do other things now if it likes
puts "All done!"
