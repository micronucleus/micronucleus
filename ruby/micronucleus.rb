require 'libusb'

# Abstracts access to micronucleus avr tiny85 bootloader - can be used only to erase and upload bytes
class Micronucleus
  Functions = [
    :get_info,
    :write_page,
    :erase_application,
    :run_program
  ]

  # return all micronucleus devices connected to computer
  def self.all
    usb = LIBUSB::Context.new
    usb.devices.select { |device|
      device.idVendor == 0x16d0 && device.idProduct == 0x0753
    }.map { |device|
      self.new(device)
    }
  end

  def initialize devref
    @device = devref
  end

  def info
    unless @info
      result = control_transfer(function: :get_info, dataIn: 4)
      flash_length, page_size, write_sleep = result.unpack('S>CC')

      @info = {
        flash_length: flash_length,
        page_size: page_size,
        pages: (flash_length.to_f / page_size).ceil,
        write_sleep: write_sleep.to_f / 1000.0,
        version: "#{@device.bcdDevice >> 8}.#{@device.bcdDevice & 0xFF}",
        version_numeric: @device.bcdDevice
      }
    end

    @info
  end

  def erase!
    puts "erasing"
    info = self.info
    control_transfer(function: :erase_application)
    info[:pages].times do
      sleep(info[:write_sleep]) # sleep for as many pages as the chip has to erase
    end
    puts "erased chip"
  end

  # upload a new program
  def program= bytestring
    info = self.info
    raise "Program too long!" if bytestring.bytesize > info[:flash_length]
    bytes = bytestring.bytes.to_a

    erase!

    address = 0
    bytes.each_slice(info[:page_size]) do |slice|
      slice.push(0xFF) while slice.length < info[:page_size] # ensure every slice is one page_size long - pad out if needed
      
      puts "uploading @ #{address} of #{bytes.length}"
      control_transfer(function: :write_page, wIndex: address, wValue: slice.length, dataOut: slice.pack('C*'))
      sleep(info[:write_sleep])
      address += slice.length
    end
  end

  def finished
    info = self.info

    puts "asking device to finish writing"
    control_transfer(function: :run_program)
    puts "waiting for device to finish"

    # sleep for as many pages as the chip could potentially need to write - this could be smarter
    info[:pages].times do
      sleep(info[:write_sleep]) 
    end

    @io.close
    @io = nil
  end

  def inspect
    "<MicroBoot #{info[:version]}: #{(info[:flash_length] / 1024.0).round(1)} kb programmable>"
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
  
  # calculate usb request type
  def usb_request_type opts #:nodoc:
    value = LIBUSB::REQUEST_TYPE_VENDOR | LIBUSB::RECIPIENT_DEVICE
    value |= LIBUSB::ENDPOINT_OUT if opts.has_key? :dataOut
    value |= LIBUSB::ENDPOINT_IN if opts.has_key? :dataIn
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