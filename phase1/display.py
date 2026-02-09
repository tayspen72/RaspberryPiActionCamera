import fcntl
from font import get_character, CHARACTER_WIDTH

I2C_BUS = "/dev/i2c-1"
SSD1306_ADDR = 0x3C

DEFAULT_COLS = 128
DEFAULT_ROWS = 32
_CMD_CONTRAST              = 0x81
_CMD_DISPLAY_FORCE_ON_OFF  = 0xA4   # +0 = follow RAM, +1 = all on
_CMD_DISPLAY_INVERT        = 0xA6   # +0 = normal,    +1 = inverted
_CMD_DISPLAY_ON_OFF        = 0xAE   # +0 = off,       +1 = on
_CMD_MEMORY_ADDRESSING     = 0x20
_CMD_COLUMN_ADDRESS        = 0x21
_CMD_PAGE_ADDRESS          = 0x22
_CMD_DISPLAY_START_LINE    = 0x40
_CMD_SEGMENT_REMAP         = 0xA0   # +0 = normal,    +1 = reverse
_CMD_MULTIPLEX_RATIO       = 0xA8
_CMD_COM_SCAN_DIRECTION    = 0xC0   # +0 = normal,    +8 = remapped
_CMD_DISPLAY_OFFSET        = 0xD3
_CMD_COM_PINS_CONFIG       = 0xDA
_CMD_DISPLAY_CLOCK         = 0xD5
_CMD_PRECHARGE_PERIOD      = 0xD9
_CMD_VCOM_DESELECT         = 0xDB
_CMD_CHARGE_PUMP           = 0x8D

def _ctrl(continuation: bool, data: bool) -> int:
    return ((1 if continuation else 0) << 7) | ((1 if data else 0) << 6)


class Display:
    def __init__(self, cols: int = DEFAULT_COLS, rows: int = DEFAULT_ROWS):
        self.cols = cols
        self.rows = rows
        self._buffer = bytearray(cols * rows // 8)
        self._i2c_fd = None
        self._open()
        self._init_panel()
        self.clear()

    def show_mode(self, mode_index: int):
        MODE_COLS = (0, 40, 87)
        MODE_LABELS = ("EVENT", "RECORD", "STANDBY")
        MODE_PAGE = 0

        self.clear_page(MODE_PAGE)

        event_fg = False if mode_index == 0 else True
        event_bg = True if mode_index == 0 else False
        record_fg = False if mode_index == 1 else True
        record_bg = True if mode_index == 1 else False
        standby_fg = False if mode_index == 2 else True
        standby_bg = True if mode_index == 2 else False

        self._write_string(page=MODE_PAGE, col=MODE_COLS[0], text=MODE_LABELS[0], fg=event_fg, bg=event_bg)
        self._write_string(page=MODE_PAGE, col=MODE_COLS[1], text=MODE_LABELS[1], fg=record_fg, bg=record_bg)
        self._write_string(page=MODE_PAGE, col=MODE_COLS[2], text=MODE_LABELS[2], fg=standby_fg, bg=standby_bg)

        self._draw_buffer()

    def show_state(self, state_index: int):
        STATE_LABELS = (
            ("    WAITING FOR EVENT    "),
            ("       SAVING EVENT      "),
            ("        RECORDING        "),
            ("         STOPPED         "),
            ("                         ")
        )
        STATE_PAGE = 3

        self.clear_page(STATE_PAGE)

        self._write_string(page=STATE_PAGE, col=0, text=STATE_LABELS[state_index], fg=True, bg=False)

        self._draw_partial_buffer(STATE_PAGE, STATE_PAGE)

    def clear(self):
        for i in range(len(self._buffer)):
            self._buffer[i] = 0x00

    def clear_page(self, page: int):
        if 0 <= page < self.rows // 8:
            start_index = page * self.cols
            end_index = start_index + self.cols
            for i in range(start_index, end_index):
                self._buffer[i] = 0x00 

    def _write_pixel(self, x: int, y: int, on: bool):
        if not (0 <= x < self.cols and 0 <= y < self.rows):
            return
        page   = y // 8
        bit    = y % 8
        index  = page * self.cols + x
        if on:
            self._buffer[index] |=  (1 << bit)
        else:
            self._buffer[index] &= ~(1 << bit)

    def _write_character(self, page: int, col: int, char: str, fg: bool = True, bg: bool = False):
        glyph = get_character(char)
        if glyph is None:
            return                          # character not in font, skip
        if col + CHARACTER_WIDTH > self.cols or page >= self.rows // 8:
            return                          # would overflow the panel

        base = page * self.cols + col
        for i, col_byte in enumerate(glyph):
            out = 0x00
            for bit in range(8):
                if col_byte & (1 << bit):
                    out |= (1 if fg else 0) << bit
                else:
                    out |= (1 if bg else 0) << bit
            self._buffer[base + i] = out

    def _write_string(self, page: int, col: int, text: str, fg: bool = True, bg: bool = False):
        for ch in text.upper():
            if col + CHARACTER_WIDTH > self.cols:
                break                       # no room for another character
            self._write_character(page, col, ch, fg, bg)
            col += CHARACTER_WIDTH

    def _draw_buffer(self):
        self._send_command(bytes([_CMD_COLUMN_ADDRESS, 0, self.cols - 1]))
        self._send_command(bytes([_CMD_PAGE_ADDRESS,   0, (self.rows // 8) - 1]))
        # Data burst: control byte (no continuation, data flag) + full buffer
        payload = bytes([_ctrl(False, True)]) + bytes(self._buffer)
        self._i2c_write(payload)

    def _draw_partial_buffer(self, page_start: int, page_end: int):
        self._send_command(bytes([_CMD_COLUMN_ADDRESS, 0, self.cols - 1]))
        self._send_command(bytes([_CMD_PAGE_ADDRESS, page_start, page_end]))
        
        # Calculate offset and length for partial update
        start_offset = page_start * self.cols
        end_offset = (page_end + 1) * self.cols
        partial_buffer = self._buffer[start_offset:end_offset]
        
        # Data burst with partial buffer
        payload = bytes([_ctrl(False, True)]) + bytes(partial_buffer)
        self._i2c_write(payload)

    def _init_panel(self):
        self._send_command(bytes([_CMD_DISPLAY_ON_OFF]))                          # display off
        self._send_command(bytes([_CMD_DISPLAY_CLOCK,   0x80]))                   # clock: div=0, freq=8
        self._send_command(bytes([_CMD_MULTIPLEX_RATIO, self.rows - 1]))          # mux ratio
        self._send_command(bytes([_CMD_DISPLAY_OFFSET,  0x00]))                   # no vertical offset
        self._send_command(bytes([_CMD_DISPLAY_START_LINE]))                      # start line 0
        self._send_command(bytes([_CMD_CHARGE_PUMP,     0x14]))                   # charge pump enabled
        self._send_command(bytes([_CMD_MEMORY_ADDRESSING, 0x00]))                 # horizontal addressing
        self._send_command(bytes([_CMD_SEGMENT_REMAP + 1]))                       # segment remap (reversed)
        self._send_command(bytes([_CMD_COM_SCAN_DIRECTION + 0x08]))               # COM scan remapped
        self._send_command(bytes([_CMD_COM_PINS_CONFIG, 0x02]))                   # sequential, no remap
        self._send_command(bytes([_CMD_CONTRAST,        0x8F]))                   # contrast
        self._send_command(bytes([_CMD_PRECHARGE_PERIOD, 0x22]))                  # precharge phase1=2 phase2=2
        self._send_command(bytes([_CMD_VCOM_DESELECT,   0x20]))                   # VCOM level 0.77
        self._send_command(bytes([_CMD_DISPLAY_INVERT]))                          # normal (not inverted)
        self._send_command(bytes([_CMD_DISPLAY_FORCE_ON_OFF]))                    # follow RAM
        self._send_command(bytes([_CMD_DISPLAY_ON_OFF + 1]))                      # display on

    def _open(self):
        self._i2c_fd = open(I2C_BUS, "r+b", buffering=0)
        # I2C_SLAVE ioctl = 0x0703
        fcntl.ioctl(self._i2c_fd, 0x0703, SSD1306_ADDR)

    def _close(self):
        if self._i2c_fd:
            self._i2c_fd.close()
            self._i2c_fd = None

    def _send_command(self, cmd_bytes: bytes):
        self._i2c_write(bytes([_ctrl(False, False)]) + cmd_bytes)

    def _i2c_write(self, data: bytes):
        if self._i2c_fd:
            self._i2c_fd.write(data)

    def __del__(self):
        self._close()
