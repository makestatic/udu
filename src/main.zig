//
// Copyright (C) 2023, 2024, 2025 Ali Almalki <github@makestatic>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

const std = @import("std");
const human_size = @import("util.zig").human_size;
const comma_format = @import("util.zig").comma_format;
const WinSize = @import("util.zig").WinSize;
const get_terminal_size = @import("util.zig").get_terminal_size;
const get_win_size_linux = @import("util.zig").get_win_size_linux;
const get_win_size_bsd = @import("util.zig").get_win_size_bsd;
const get_win_size_windows = @import("util.zig").get_win_size_windows;

const usage = @embedFile("usage.txt");
pub const version = "1.0.1";

const Target = struct {
    os: []const u8,
    arch: []const u8,

    pub fn init() Target {
        const target = @import("builtin").target;

        const os = switch (target.os.tag) {
            .windows => "Windows",
            .linux => "Linux",
            .macos => "macOS",
            .freebsd => "FreeBSD",
            else => "Unknown",
        };

        const arch = switch (target.cpu.arch) {
            .x86_64 => "x86_64",
            .aarch64 => "aarch64",
            else => "Unknown",
        };

        return Target{ .os = os, .arch = arch };
    }
};

const Options = struct {
    verbose: bool = false,
    quiet: bool = false,
    help: bool = false,
    version: bool = false,
};

const ParsedArgs = struct {
    opts: Options,
    exclude: std.ArrayList([]const u8),
};

fn parse_args(args: []const []const u8, allocator: std.mem.Allocator) !ParsedArgs {
    var opts = Options{};
    var exclude = std.ArrayList([]const u8).initCapacity(allocator, 1) catch unreachable;

    for (args[1..]) |arg| {
        if (std.mem.startsWith(u8, arg, "-ex=")) {
            try exclude.append(allocator, arg[4..]);
            continue;
        }
        if (std.mem.eql(u8, arg, "-v") or std.mem.eql(u8, arg, "--verbose")) {
            opts.verbose = true;
        } else if (std.mem.eql(u8, arg, "-q") or std.mem.eql(u8, arg, "--quiet")) {
            opts.quiet = true;
        } else if (std.mem.eql(u8, arg, "-h") or std.mem.eql(u8, arg, "--help")) {
            opts.help = true;
        } else if (std.mem.eql(u8, arg, "--version") or std.mem.eql(u8, arg, "--v")) {
            opts.version = true;
        } else {
            if (std.mem.eql(u8, arg, args[0])) continue;
            if (std.mem.eql(u8, arg, args[1])) continue;
            return error.UnknownOption;
        }
    }

    return ParsedArgs{ .opts = opts, .exclude = exclude };
}

const Udu = struct {
    allocator: std.mem.Allocator,
    pool: std.Thread.Pool = undefined,
    wg: std.Thread.WaitGroup = .{},
    dirs_count: std.atomic.Value(usize) = std.atomic.Value(usize).init(0),
    files_count: std.atomic.Value(usize) = std.atomic.Value(usize).init(0),
    size: std.atomic.Value(u64) = std.atomic.Value(u64).init(0),
    ws: WinSize = undefined,

    const Self = @This();

    pub fn init(allocator: std.mem.Allocator) Self {
        return Self{ .allocator = allocator };
    }

    pub fn deinit(self: *Self) void {
        self.pool.deinit();
    }

    pub fn cycle(self: *Self, base_path: []const u8, exs: []const []const u8, opts: Options) !void {
        const cpu_count = std.Thread.getCpuCount() catch 8;
        try self.pool.init(.{
            .allocator = self.allocator,
            .n_jobs = @min(cpu_count * 2, 32),
        });

        if (get_terminal_size()) |ws| {
            self.ws = ws;
        } else {
            self.ws = WinSize{ .ws_row = 24, .ws_col = 80, .ws_xpixel = 0, .ws_ypixel = 0 };
        }

        const root = try self.allocator.dupe(u8, base_path);
        self.pool.spawnWg(&self.wg, process_dir, .{ self, root, exs, opts });
        self.wg.wait();
    }
};

fn process_dir(self: *Udu, path: []u8, exs: []const []const u8, opts: Options) void {
    // free passed memory
    defer self.allocator.free(path);

    var dir = std.fs.cwd().openDir(path, .{
        .access_sub_paths = true,
        .iterate = true,
    }) catch return;
    defer dir.close();

    const local_dirs: usize = 1;
    var local_files: usize = 0;
    var local_size: u64 = 0;

    var it = dir.iterate();
    while (it.next() catch null) |entry| {
        if (entry.kind == .directory) {
            var skip = false;
            for (exs) |ex| {
                if (std.mem.eql(u8, entry.name, ex)) {
                    skip = true;
                    break;
                }
            }
            if (skip) continue;

            const full_path = std.fs.path.join(self.allocator, &.{ path, entry.name }) catch continue;

            for (exs) |ex| {
                if (std.mem.eql(u8, full_path, ex)) {
                    self.allocator.free(full_path);
                    skip = true;
                    break;
                }
            }
            if (skip) continue;

            self.pool.spawnWg(&self.wg, process_dir, .{ self, full_path, exs, opts });
        } else {
            const st = dir.statFile(entry.name) catch continue;
            local_files += 1;
            local_size += st.size;

            if (opts.verbose) {
                var path_buf: [std.fs.max_path_bytes]u8 = undefined;
                const full_path = std.fmt.bufPrint(&path_buf, "{s}{c}{s}", .{ path, std.fs.path.sep, entry.name }) catch continue;

                var size_buf: [32]u8 = undefined;
                const size_str = human_size(&size_buf, st.size) catch {
                    const fallback = std.fmt.bufPrint(&size_buf, "{d}", .{st.size}) catch continue;
                    std.debug.print("{s}\n", .{fallback});
                    continue;
                };

                const term_cols = self.ws.ws_col;
                const size_col_width = @max(size_str.len, 8);
                const padding = 1;
                const available = if (term_cols > (size_col_width + padding))
                    term_cols - (size_col_width + padding)
                else
                    full_path.len;

                const truncated = full_path[0..@min(full_path.len, available)];
                std.debug.print("{s:<8} {s}\n", .{ size_str, truncated });
            }
        }
    }

    _ = self.dirs_count.fetchAdd(local_dirs, .monotonic);
    _ = self.files_count.fetchAdd(local_files, .monotonic);
    _ = self.size.fetchAdd(local_size, .monotonic);
}

pub fn main() !void {
    const target = Target.init();

    var gpa = std.heap.GeneralPurposeAllocator(.{ .thread_safe = true }){};
    defer std.debug.assert(gpa.deinit() == .ok);
    const allocator = gpa.allocator();

    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);

    if (args.len < 2) {
        std.debug.print("{s}", .{usage});
        std.process.exit(1);
    }

    var parsed = parse_args(args, allocator) catch |err| {
        std.debug.print("[ERROR]: {s}\n\n{s}", .{ @errorName(err), usage });
        std.process.exit(1);
    };
    defer parsed.exclude.deinit(allocator);

    const opts = parsed.opts;
    const exclude = parsed.exclude;

    if (opts.help) {
        std.debug.print("{s}", .{usage});
        std.process.exit(0);
    }
    if (std.mem.containsAtLeast(u8, args[1], 1, "-")) {
        if (opts.version) {
            std.debug.print("v{s} [{s}/{s}]\n", .{ version, target.os, target.arch });
            std.process.exit(0);
        }
        std.debug.print("{s}", .{usage});
        std.debug.print("\n[ERROR] first argument must be a <path>\n", .{});
        std.process.exit(1);
    }

    const path = args[1];

    var inst = Udu.init(allocator);
    defer inst.deinit();

    try inst.cycle(path, exclude.items, opts);

    const size = inst.size.load(.monotonic);
    var buf_dirs: [32]u8 = undefined;
    var buf_files: [32]u8 = undefined;
    var buf_size: [32]u8 = undefined;
    var buf_mb: [64]u8 = undefined;

    std.debug.print(
        \\
        \\ ------------------------------------------
        \\¦ Directories  ¦  {s}
        \\ ------------------------------------------
        \\¦ Files        ¦  {s}
        \\ ------------------------------------------
        \\¦ Total        ¦  {s} ({s}) 
        \\ ------------------------------------------
        \\
    , .{
        try comma_format(&buf_dirs, inst.dirs_count.load(.monotonic)),
        try comma_format(&buf_files, inst.files_count.load(.monotonic)),
        try comma_format(&buf_size, size),
        try human_size(&buf_mb, size),
    });
}
