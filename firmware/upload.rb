require 'libusb'

# Abstracts access to uBoot avr tiny85 bootloader - can be used only to upload bytes
class MicroBoot
  Functions = [
    :get_info,
    :write_page,
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
      result = control_transfer(function: :get_info, dataIn: 5)
      version, flash_length, page_size, write_sleep = result.unpack('CS>CC')

      @info = {
        flash_length: flash_length,
        page_size: page_size,
        version: version,
        write_sleep: write_sleep.to_f / 1000.0
      }
    end
    @info
  end
  
  # upload a new program
  def program= bytestring
    raise "Program too long!" if bytestring.bytesize > info[:flash_length]
    bytes = bytestring.bytes.to_a
    
    address = 0
    bytes.each_slice(info[:page_size]) do |bytes|
      control_transfer(function: :write_page, wIndex: address, wValue: bytes.length, dataOut: bytes.pack('C*'))
      sleep(info[:write_sleep])
      address += bytes.length
    end
  end
  
  def finished
    control_transfer(function: :run_program)
    sleep(info[:write_sleep]) # not sure if this is worth having? It's okay if USB fails now...
    
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

puts "Finding devices"
thinklets = MicroBoot.all
puts "Found #{thinklets.length} thinklet"
exit unless thinklets.length > 0

thinklet = thinklets.first

puts "First thinklet: #{thinklet.info.inspect}"

test_data = ("---- Hello World! ----" * 1).encode("BINARY")
puts "Attempting to write '#{test_data}' to first thinklet's program memory"
thinklet.program = test_data

puts "That seems to have gone well! Telling thinklet to run program..."

sleep(0.5)

thinklet.finished # let thinklet know it can go do other things now if it likes
puts "All done!"
