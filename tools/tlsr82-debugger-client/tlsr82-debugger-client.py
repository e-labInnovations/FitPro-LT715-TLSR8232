import argparse
import serial
import time

BAUD = 115200 * 2

SLEEP_AFTER_RESET_IN_S = 0.02
# This is modified when reading RAM and flash.
SLEEP_BETWEEN_READ_AND_WRITE_IN_S = 0.02

RAM_SIZE = 0x010000
FLASH_SIZE = 0x7d000

# The largest write payload known to survive the round trip on real hardware.
#
# Bigger chunks are the single best lever on flash time — eleven of a chunk's
# twelve transactions are fixed cost — but the bridge truncates large replies in
# a way that is not yet understood. Two measurements, both exactly ten bytes
# short of the expected 2x:
#
#   256-byte payload (261-byte request)  wanted 522, got 512
#   128-byte payload (133-byte request)  wanted 266, got 256
#
# Ten is 2 x (4 header + 1 trailer), which is suggestive, but the header is
# plainly echoed at the front of both replies, so the simple explanations do not
# hold. Rather than guess again, `probe_chunk` measures the real ceiling against
# the bridge in front of you. 16 is what the tool has always used and what every
# small transaction in a failed run still validated cleanly.
BRIDGE_MAX_CHUNK = 16

serial_port = None


def hexdump(bytearr):
    return ' '.join(f'{b:02x}' for b in bytearr)


def make_read_request(addr, n_bytes):
    return [0x5a, addr >> 8, addr & 0xff, 0x80] + [0xff] * (n_bytes + 1)


def make_write_request(addr, data):
    return [0x5a, addr >> 8, addr & 0xff, 0x00] + data + [0xff]


def request_payload_size(request):
    "Discounts the leading 4 bytes and trailing 1 byte from read and write requests."
    return len(request) - 5


def validate_raw_response(request, raw_response):
    return all([
        raw_response[0] == 0x02,
        raw_response[1] == request[0],
        raw_response[2] == 0x00,
        raw_response[3] == request[1],
        raw_response[4] == 0x00,
        raw_response[5] == request[2],
        raw_response[6] == 0x00,
        raw_response[7] == request[3],
        raw_response[-2] == 0x02,
        raw_response[-1] == 0xff,
    ])


def parse_response(raw_response, n_bytes):
    "Extracts the data bytes from the raw response."
    return [
        raw_response[(i+4)*2 + 1]
        for i in range(n_bytes)
    ]


def write_and_read(data, expect=None):
    """
    Sends a request and returns the bridge's raw reply.

    The bridge answers with exactly two bytes for every byte it receives — both
    validate_raw_response() and parse_response() already depend on that — so
    whenever the caller knows the request length it also knows the reply length.
    Given that, `expect` reads precisely that many bytes and returns the moment
    the last one lands.

    The alternative, and what this used to do unconditionally, is to sleep a
    fixed interval and then read_all() whatever turned up. That pays the worst
    case on every transaction, and writing 96 KB in 16-byte chunks is roughly
    74000 of them: about six minutes of pure time.sleep() before accounting for
    a single byte on the wire. It is also silently wrong for large payloads,
    where the reply takes longer to arrive than the sleep lasts and read_all()
    returns a truncated buffer.

    Callers that cannot predict the length (the 0x55 command channel) still get
    the sleep.
    """
    # Any bytes still sitting here are from a transaction that went wrong;
    # keeping them would shift every subsequent reply by their length.
    serial_port.reset_input_buffer()
    serial_port.write(data)

    if expect is None:
        time.sleep(SLEEP_BETWEEN_READ_AND_WRITE_IN_S)
        return serial_port.read_all()

    raw = serial_port.read(expect)
    if len(raw) < expect:
        raise Exception(
            f"Short reply: wanted {expect} bytes, got {len(raw)} before the "
            f"{serial_port.timeout}s timeout.\n\t{hexdump(raw)}")
    return raw


def write_and_read_cmd(cmd, data=None):
    cmd_bytes = bytearray([0x55, cmd] if data is None else [0x55, cmd] + data)
    return write_and_read(cmd_bytes)


def write_and_read_data(request):
    payload_size = request_payload_size(request)
    data_bytes = bytearray(request)
    raw_response = write_and_read(data_bytes, expect=2 * len(data_bytes))
    if not validate_raw_response(request, raw_response):
        raise Exception(
            f"Invalid request/raw response pair:\n\t{hexdump(request)}\n\t{hexdump(raw_response)}")
    return parse_response(raw_response, payload_size)


def get_soc_id():
    res = write_and_read_data(make_read_request(0x007e, 2))
    return res[1] << 8 | res[0]
    # res = write_and_read_data(make_read_request(0x007e, 1))
    # return res[0]


def set_speed(speed):
    """The SWS speed is set in the 0x00b2 register by means of
        specifying the number of clocks per bit."""
    # return write_and_read_data(make_write_request(0x00b0, [0x00, 0x80, speed, 0x00]))
    return write_and_read_data(make_write_request(0x00b2, [speed]))


def set_pgm_speed(speed):
    return write_and_read_cmd(0x05, [speed])


def find_suitable_sws_speed():
    for speed in range(2, 0x7f):
        print(f'Trying speed {speed}')
        set_speed(speed)
        # Try to make a read request. If we get a valid response, assume we've
        # found a suitable SWS speed.
        try:
            get_soc_id()
        except Exception:
            continue
        else:
            print(f'Found and set suitable SWS speed: {speed}')
            return speed
    raise RuntimeError("Unable to find a suitable SPI speed")


def send_flash_write_enable():
    # CNS low.
    write_and_read_data(make_write_request(0x0d, [0x00]))
    # Write enable.
    write_and_read_data(make_write_request(0x0c, [0x06]))
    # CNS high.
    write_and_read_data(make_write_request(0x0d, [0x01]))


def send_flash_erase():
    # CNS low.
    write_and_read_data(make_write_request(0x0d, [0x00]))
    # Write enable.
    write_and_read_data(make_write_request(0x0c, [0x60]))
    # CNS high.
    write_and_read_data(make_write_request(0x0d, [0x01]))


def send_flash_get_status():
    # CNS low.
    write_and_read_data(make_write_request(0x0d, [0x00]))
    # Get flash status command.
    write_and_read_data(make_write_request(0x0c, [0x05]))
    # Start SPI.
    write_and_read_data(make_write_request(0x0c, [0xff]))
    # Read the status byte.
    res = write_and_read_data(make_read_request(0x0c, 1))
    # CNS high.
    write_and_read_data(make_write_request(0x0d, [0x01]))
    return res


def send_cpu_stop():
    return write_and_read_data(make_write_request(0x0602, [0x05]))


def send_csn_high():
    return write_and_read_data(make_write_request(0x000d, [0x01]))


def send_csn_low():
    return write_and_read_data(make_write_request(0x000d, [0x00]))


def dump_ram():
    contents = []
    CHUNK_SIZE = 32
    for addr in range(0x00, RAM_SIZE, CHUNK_SIZE):
        # Report progress.
        if addr & 0xff == 0:
            print(f'0x{addr:04x} {100 * addr / RAM_SIZE:05.2f}%')
        contents.extend(write_and_read_data(
            make_read_request(addr, CHUNK_SIZE)))
    return contents


def read_flash(addr, chunk_size):
    contents = []
    # send_flash_write_enable()
    # CNS low.
    write_and_read_data(make_write_request(0x0d, [0x00]))
    # Read command.
    write_and_read_data(make_write_request(0x0c, [0x03]))
    write_and_read_data(make_write_request(0x0c, [(addr >> 16) & 0xff]))
    write_and_read_data(make_write_request(0x0c, [(addr >> 8) & 0xff]))
    write_and_read_data(make_write_request(0x0c, [addr & 0xff]))

    # FIFO mode.
    write_and_read_data(make_write_request(0xb3, [0x80]))

    for i in range(chunk_size):
        write_and_read_data(make_write_request(0x0c, [0xff]))
        res = write_and_read_data(make_read_request(0x0c, 1))
        assert len(res) == 1
        contents.extend(res)

    # RAM mode.
    write_and_read_data(make_write_request(0xb3, [0x00]))

    # CNS high.
    write_and_read_data(make_write_request(0x0d, [0x01]))
    return contents


def write_flash(addr, data):
    send_flash_write_enable()

    # CNS low.
    write_and_read_data(make_write_request(0x0d, [0x00]))

    # Write command.
    write_and_read_data(make_write_request(0x0c, [0x02]))
    write_and_read_data(make_write_request(0x0c, [(addr >> 16) & 0xff]))
    write_and_read_data(make_write_request(0x0c, [(addr >> 8) & 0xff]))
    write_and_read_data(make_write_request(0x0c, [addr & 0xff]))

    # FIFO mode.
    write_and_read_data(make_write_request(0xb3, [0x80]))

    # Write data
    # CPU stop?
    write_and_read_data(make_write_request(0x0c, data))

    # # RAM mode.
    write_and_read_data(make_write_request(0xb3, [0x00]))

    # CNS high.
    write_and_read_data(make_write_request(0x0d, [0x01]))


def dump_flash(debug):
    contents = []
    CHUNK_SIZE = 16
    for addr in range(0x00, FLASH_SIZE, CHUNK_SIZE):
        # Report progress.
        if addr & 0xff == 0:
            print(f'0x{addr:06x} {100 * addr / FLASH_SIZE:05.2f}%')
        # Retry the same address in case something goes wrong.
        while True:
            try:
                res = read_flash(addr, CHUNK_SIZE)
                if debug:
                    print(f'Read: {hexdump(res)}')
                contents.extend(res)
                break
            except Exception as e:
                print(f"Retrying 0x{addr:08x}... {e}")

    return contents


def write_to_file(filename, contents):
    print(f"Writing {len(contents)} bytes to {filename}")
    with open(filename, 'wb') as f:
        f.write(bytes(contents))


def init_soc(sws_speed=None):
    # Set RST to low - turns of the SoC.
    write_and_read_cmd(0x00)
    # Set RST high - starts to turn on the SoC.
    # write_and_read_cmd(0x01)
    # Give some time for the reset capacitor to charge and turn the chip on.
    time.sleep(SLEEP_AFTER_RESET_IN_S)

    # Send an "activate" command. The STM32 will receive this command and put the Telink
    # in a suitable state. The STM32 will stop the Telink CPU by writing the value
    # 0x05 to Telink's 0x0602 register. It will also set a default SWS speed, but we
    # will override it later when we find a suitable SWS speed.
    write_and_read_cmd(0x02, [0x00, 0xf0])

    set_pgm_speed(0x03)
    if sws_speed is not None:
        set_speed(sws_speed)
    else:
        find_suitable_sws_speed()


def dump_flash_main(args):
    init_soc(args.sws_speed)
    print(f'Dumping flash to {args.filename}...')
    # Speed things up a little bit.
    global SLEEP_BETWEEN_READ_AND_WRITE_IN_S
    SLEEP_BETWEEN_READ_AND_WRITE_IN_S = 0.001
    write_to_file(args.filename, dump_flash(args.debug))


def erase_flash_main(args):
    init_soc(args.sws_speed)
    print(f'Erasing flash...')
    send_flash_write_enable()
    send_flash_erase()
    while True:
        res = send_flash_get_status()
        print(f'Flash status: {hexdump(res)}')
        if res[0] == 0:
            break
        time.sleep(1)

    # CNS high.
    # write_and_read_data(make_write_request(0x0d, [0x01]))


def write_flash_main(args):
    init_soc(args.sws_speed)
    time.sleep(0.02)

    print(f'Erasing flash...')
    send_flash_write_enable()
    send_flash_erase()
    while True:
        res = send_flash_get_status()
        print(f'Flash status: {hexdump(res)}')
        if res[0] == 0:
            break
        time.sleep(1)

    print(f'Writing flash from {args.filename}...')
    global SLEEP_BETWEEN_READ_AND_WRITE_IN_S
    SLEEP_BETWEEN_READ_AND_WRITE_IN_S = 0.005

    """
    Chunk size is the dominant cost, because only one of the twelve transactions
    a chunk costs carries any data — the other eleven set CSN, issue the write
    command, and clock out the three address bytes, and they cost the same
    whether the chunk holds 16 bytes or 256.

    256 is the ceiling, set by the flash itself: page program (0x02) wraps
    within a 256-byte page rather than carrying on into the next one, so a chunk
    that straddles a boundary would write its tail back over its own head.
    Addresses start at zero and advance by the chunk size, so any power of two
    up to 256 stays inside a page.
    """
    """
    A power of two so chunks stay inside a 256-byte flash page: page program
    wraps within a page rather than carrying on into the next one, and addresses
    start at zero and advance by the chunk size.

    Sizes above BRIDGE_MAX_CHUNK are allowed but warned about — run `probe_chunk`
    to find what this particular bridge really tolerates.
    """
    chunk_size = args.chunk_size
    if chunk_size < 1 or chunk_size > 256 or chunk_size & (chunk_size - 1):
        raise SystemExit(
            f"--chunk-size must be a power of two and at most 256 (the flash "
            f"page size), got {chunk_size}")
    if chunk_size > BRIDGE_MAX_CHUNK:
        print(f"Warning: chunk size {chunk_size} is above the {BRIDGE_MAX_CHUNK} "
              f"known to work. If this dies with a short reply, run probe_chunk.")

    started = time.time()
    written = 0
    skipped = 0

    with open(args.filename, 'rb') as f:
        contents = f.read()
        size = len(contents)
        for addr in range(0x00, size, chunk_size):
            if addr % 0x1000 == 0:
                print(f'0x{addr:06x} {100 * addr / size:05.2f}%')
            data = contents[addr:min(addr + chunk_size, size)]
            # Erase already left every byte at 0xff, so a chunk that is all
            # padding is a chunk already correct on the device.
            if all(b == 0xff for b in data):
                skipped += len(data)
                continue
            if args.debug:
                print(f'writing: {hexdump(data)}')
            write_flash(addr, list(data))
            written += len(data)

    elapsed = time.time() - started
    print(f'Wrote {written} bytes in {elapsed:.1f}s '
          f'({written / elapsed / 1024:.1f} KB/s), skipped {skipped} erased.')

    while True:
        res = send_flash_get_status()
        print(f'Flash status: {hexdump(res)}')
        if res[0] == 0:
            break
        time.sleep(1)
    # Set RST to low - turns off the SoC.
    write_and_read_cmd(0x00)
    # Set RST to high - turns on the SoC.
    write_and_read_cmd(0x01)


def dump_ram_main(args):
    init_soc(args.sws_speed)
    print(f'Dumping ram to {args.filename}...')
    # Speed things up a little bit.
    global SLEEP_BETWEEN_READ_AND_WRITE_IN_S
    SLEEP_BETWEEN_READ_AND_WRITE_IN_S = 0.00
    write_to_file(args.filename, dump_ram())


def get_soc_id_main(args):
    init_soc(args.sws_speed)
    print(f'SOC ID: 0x{get_soc_id():04x}')


def probe_chunk_main(args):
    """
    Measures the largest request this bridge answers in full.

    Chunk size dominates flash time, but guessing it costs a failed write and a
    re-erase. This asks the hardware instead. Read requests only — nothing is
    written, and CSN is left high so the flash never sees a command.
    """
    init_soc(args.sws_speed)
    send_csn_high()

    print(f'{"payload":>8} {"request":>8} {"expected":>9} {"received":>9}')
    best = 0
    for payload in (1, 8, 16, 24, 32, 48, 64, 96, 128, 192, 251):
        request = make_read_request(0x0c, payload)
        expected = 2 * len(request)

        serial_port.reset_input_buffer()
        serial_port.write(bytearray(request))
        # Generous and fixed: the point is to see everything the bridge sends,
        # not to be quick about it.
        time.sleep(0.3)
        received = len(serial_port.read_all())

        print(f'{payload:8d} {len(request):8d} {expected:9d} {received:9d}'
              f'  {"ok" if received == expected else "SHORT by " + str(expected - received)}')
        if received == expected:
            best = payload

    print(f'\nLargest payload answered in full: {best}')
    usable = 1
    while usable * 2 <= best:
        usable *= 2
    print(f'Largest usable --chunk-size (power of two, <=256): {min(usable, 256)}')
    if usable > BRIDGE_MAX_CHUNK:
        print(f'That is above the built-in default of {BRIDGE_MAX_CHUNK}; pass '
              f'--chunk-size {min(usable, 256)} to write_flash to use it.')


def cpu_run_main(args):
    init_soc(args.sws_speed)
    # Tell CPU to run.
    write_and_read_data(make_write_request(0x0602, [0x88]))


def main():
    args_parser = argparse.ArgumentParser(description='TLSR')
    args_parser.add_argument('--serial-port', type=str, required=True,
                             help="Serial port to use - this should be the USB CDC port that is connected to the STM32 (e.g.: /dev/cu.usbmodem6D8E448E55511.")
    args_parser.add_argument(
        '--sws-speed', type=int, help="SWS speed in the range [0x02, 0x7f]. If not provided, the script will try to find a suitable SWS speed automatically.")
    args_parser.add_argument(
        '--debug', action="store_true", help="Enabled debugging information.")
    args_parser.add_argument(
        '--baud', type=int, default=BAUD,
        help=f"Serial rate to the STM32 bridge. Default: {BAUD}. The link is "
             "USB CDC, so the bridge may well ignore this and run at full USB "
             "speed regardless — worth trying 921600 or 2000000.")
    subparsers = args_parser.add_subparsers(dest="cmd", required=True)

    dump_flash_parser = subparsers.add_parser('dump_flash')
    dump_flash_parser.set_defaults(func=dump_flash_main)
    dump_flash_parser.add_argument('filename', type=str)

    dump_ram_parser = subparsers.add_parser('dump_ram')
    dump_ram_parser.set_defaults(func=dump_ram_main)
    dump_ram_parser.add_argument('filename', type=str)

    get_soc_id_parser = subparsers.add_parser('get_soc_id')
    get_soc_id_parser.set_defaults(func=get_soc_id_main)

    write_flash_parser = subparsers.add_parser('write_flash')
    write_flash_parser.set_defaults(func=write_flash_main)
    write_flash_parser.add_argument('filename', type=str)
    write_flash_parser.add_argument(
        '--chunk-size', type=int, default=BRIDGE_MAX_CHUNK,
        help=f"Bytes per page-program. Power of two, 256 max (the flash page "
             f"size). Larger is much faster but the bridge truncates large "
             f"replies; run probe_chunk to find this one's real ceiling. "
             f"Default: {BRIDGE_MAX_CHUNK}.")

    erase_flash_parser = subparsers.add_parser('erase_flash')
    erase_flash_parser.set_defaults(func=erase_flash_main)

    erase_flash_parser = subparsers.add_parser('cpu_run')
    erase_flash_parser.set_defaults(func=cpu_run_main)

    probe_chunk_parser = subparsers.add_parser('probe_chunk')
    probe_chunk_parser.set_defaults(func=probe_chunk_main)

    args = args_parser.parse_args()

    # Initialize the serial port
    global serial_port
    # A read timeout matters now that replies are read by length rather than by
    # elapsed time: without one, a single dropped byte blocks forever instead of
    # raising. Generous enough for a 256-byte page's 517-byte reply.
    serial_port = serial.Serial(
        args.serial_port, args.baud, serial.EIGHTBITS, serial.PARITY_NONE,
        serial.STOPBITS_ONE, timeout=2.0)

    args.func(args)


if __name__ == "__main__":
    main()
