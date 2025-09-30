const std = @import("std");

pub fn human_size(buf: []u8, bytes: u64) ![]u8 {
    const KB: f64 = 1024.0;
    const MB = KB * 1024.0;
    const GB = MB * 1024.0;
    const TB = GB * 1024.0;

    const fbytes = @as(f64, @floatFromInt(bytes));

    if (bytes >= @as(u64, @intFromFloat(TB)))
        return std.fmt.bufPrint(buf, "{d:.2}TB", .{fbytes / TB});
    if (bytes >= @as(u64, @intFromFloat(GB)))
        return std.fmt.bufPrint(buf, "{d:.2}GB", .{fbytes / GB});
    if (bytes >= @as(u64, @intFromFloat(MB)))
        return std.fmt.bufPrint(buf, "{d:.2}MB", .{fbytes / MB});
    if (bytes >= 1024)
        return std.fmt.bufPrint(buf, "{d:.2}KB", .{fbytes / KB});
    return std.fmt.bufPrint(buf, "{d}B", .{bytes});
}

pub fn comma_format(buf: []u8, value: u64) ![]u8 {
    if (value == 0) return std.fmt.bufPrint(buf, "0", .{});

    var temp = value;
    var digit_count: usize = 0;
    while (temp > 0) : (temp /= 10) digit_count += 1;

    const comma_count = (digit_count - 1) / 3;
    const total_len = digit_count + comma_count;

    var pos = total_len;
    temp = value;
    var digits_since_comma: usize = 0;

    while (temp > 0) {
        if (digits_since_comma == 3) {
            pos -= 1;
            buf[pos] = ',';
            digits_since_comma = 0;
        }
        pos -= 1;
        buf[pos] = '0' + @as(u8, @intCast(temp % 10));
        temp /= 10;
        digits_since_comma += 1;
    }

    return buf[0..total_len];
}

pub const WinSize = struct {
    ws_row: u16,
    ws_col: u16,
    ws_xpixel: u16,
    ws_ypixel: u16,
};

pub fn get_win_size_linux() ?WinSize {
    var ws: WinSize = undefined;
    const TIOCGWINSZ: c_ulong = 0x5413;
    if (std.c.ioctl(std.c.STDOUT_FILENO, TIOCGWINSZ, &ws) < 0) return null;
    return ws;
}

pub fn get_win_size_bsd() ?WinSize {
    var ws: WinSize = undefined;
    const TIOCGWINSZ: c_ulong = 0x40087468;
    if (std.c.ioctl(std.c.STDOUT_FILENO, TIOCGWINSZ, &ws) < 0) return null;
    return ws;
}

pub fn get_win_size_windows() ?WinSize {
    const kernel32 = std.os.windows.kernel32;
    var info: std.os.windows.CONSOLE_SCREEN_BUFFER_INFO = undefined;
    const h = kernel32.GetStdHandle(std.os.windows.STD_OUTPUT_HANDLE);
    if (kernel32.GetConsoleScreenBufferInfo(h, &info) == 0) return null;

    return WinSize{
        .ws_row = @intCast(info.srWindow.Bottom - info.srWindow.Top + 1),
        .ws_col = @intCast(info.srWindow.Right - info.srWindow.Left + 1),
        .ws_xpixel = 0,
        .ws_ypixel = 0,
    };
}

pub fn get_terminal_size() ?WinSize {
    return switch (@import("builtin").os.tag) {
        .linux => get_win_size_linux(),
        .macos, .freebsd => get_win_size_bsd(),
        .windows => get_win_size_windows(),
        else => null,
    };
}
